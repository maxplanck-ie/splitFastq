#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <getopt.h>
#include <assert.h>

int isDuplicate(char *line) {
    char *p;
    size_t l = strlen(line) - 11;

    p = line + l;
    if(strcmp(p, " duplicate\n") == 0) {
        //Strip " duplicate" from the end, fixing the line ending
        *(p++) = '\n';
        *p = 0;
        return 1;
    }
    return 0;
}

int processSingle(FILE *ifile, char *bname, char *R1, char *extension, uint64_t *total, uint64_t *dupes, char* pigz_threads) {
    char *cmd = NULL;
    FILE *o1=NULL, *o1d=NULL;
    FILE *out;
    char *line = malloc(1024);
    if(!line) return 1;

    //Non-duplicate output
    cmd = calloc(strlen(bname) + strlen(R1) + strlen(extension) + strlen(" pigz -p >  ") + strlen(pigz_threads), sizeof(char));
    if(!cmd) return 1; 
    sprintf(cmd, "pigz -p %s > %s%s%s", pigz_threads, bname, R1, extension);
    o1 = popen(cmd, "w");
    free(cmd);

    //duplicate output
    cmd = calloc(strlen(bname) + strlen(R1) + strlen(extension) + strlen("_optical_duplicates pigz -p >  ") + strlen(pigz_threads), sizeof(char));
    if(!cmd) return 1;
    sprintf(cmd, "pigz -p %s > %s%s_optical_duplicates%s", pigz_threads, bname, R1, extension);
    o1d = popen(cmd, "w");
    free(cmd);

    *total = 0;
    *dupes = 0;
    //Iterate through the lines, writing as appropriate
    while((line = fgets(line, 1024, ifile))) {
        // Check if this is a duplicate
        if(isDuplicate(line)) {
             *dupes += 1;
             out = o1d;
        } else {
             out = o1;
        }
        //Read 1
        fputs(line, out);
        if(!(line = fgets(line, 1024, ifile))) return 1;
        fputs(line, out);
        if(!(line = fgets(line, 1024, ifile))) return 1;
        fputs(line, out);
        if(!(line = fgets(line, 1024, ifile))) return 1;
        fputs(line, out);
        *total += 1;
    }
    free(line);

    return 0;
}
    
int processPaired(FILE *ifile, char *bname, char *R1, char *R2, char *extension, uint64_t *total, uint64_t *dupes, char *pigz_threads) {
    char *cmd = NULL;
    FILE *o1=NULL, *o2=NULL, *o1d=NULL, *o2d=NULL;
    FILE *out1, *out2;
    char *line = malloc(1024);
    if(!line) return 1;

    //Non-duplicate output
    cmd = calloc(strlen(bname) + strlen(R1) + strlen(extension) + strlen(" pigz -p >  ") + strlen(pigz_threads), sizeof(char));
    if(!cmd) return 1; 
    sprintf(cmd, "pigz -p %s > %s%s%s", pigz_threads, bname, R1, extension);
    o1 = popen(cmd, "w");
    sprintf(cmd, "pigz -p %s > %s%s%s", pigz_threads, bname, R2, extension);
    o2 = popen(cmd, "w");
    free(cmd);

    //duplicate output
    cmd = calloc(strlen(bname) + strlen(R1) + strlen(extension) + strlen("_optical_duplicates pigz -p >  ") + strlen(pigz_threads), sizeof(char));
    if(!cmd) return 1; 
    sprintf(cmd, "pigz -p %s > %s%s_optical_duplicates%s", pigz_threads, bname, R1, extension);
    o1d = popen(cmd, "w");
    sprintf(cmd, "pigz -p %s > %s%s_optical_duplicates%s", pigz_threads, bname, R2, extension);
    o2d = popen(cmd, "w");
    free(cmd);

    *total = 0;
    *dupes = 0;
    //Iterate through the lines, writing as appropriate
    while((line = fgets(line, 1024, ifile)) != NULL) {
        // Check if this is a duplicate
        if(isDuplicate(line)) {
             *dupes += 1;
             out1 = o1d;
             out2 = o2d;
        } else {
             out1 = o1;
             out2 = o2;
        }
        //Read 1
        fputs(line, out1);
        if(!(line = fgets(line, 1024, ifile))) return 1;
        fputs(line, out1);
        if(!(line = fgets(line, 1024, ifile))) return 1;
        fputs(line, out1);
        if(!(line = fgets(line, 1024, ifile))) return 1;
        fputs(line, out1);
        //Read 2
        if(!(line = fgets(line, 1024, ifile))) return 1;
        fputs(line, out2);
        if(!(line = fgets(line, 1024, ifile))) return 1;
        fputs(line, out2);
        if(!(line = fgets(line, 1024, ifile))) return 1;
        fputs(line, out2);
        if(!(line = fgets(line, 1024, ifile))) return 1;
        fputs(line, out2);
        *total += 1;
    }
    free(line);

    return 0;
}


void usage(char *prog) {
    printf("Usage: %s [OPTIONS] <input> <basename>\n", prog);
    printf("Splits an interleaved single or paired-end file into 2/4 output files according\n\
to whether 'duplicate' is in the read name. This is useful to split the output\n\
of clumpify into duplicates and non-duplicates. The metrics are then written to\n\
basename.deduplicate.txt and files to basename_R1.fastq.gz,\n\
basename_R1_optical_duplicates.fastq.gz and so on. The R1/R2 designator and the read extension can be changed.\n\n\
input:         Interleaved input file from clumpify.\n\
basename:      The basename for the output files.\n\
\nOptions:\n\
--SE:          (optional) Input is single-end rather than paired-end.\n\
--R1:          (optional) Designator for read 1. Default: _R1\n\
--R2:          (optional) Designator for read 2. Default: _R2\n\
--extension:   (optional) File extension. Default: .fastq.gz\n\
--pigzThreads: (optional) Number of threads used by each pigz process. Note that there can be 4 of these.\n");
}


int main(int argc, char *argv[]) {
    int PE = 1, rv, c;
    char *bname, *input, *cmd=NULL;
    uint64_t total=0, dupes=0;
    char *pigz_threads = "1";
    FILE *ifile;
    char *R1 = "_R1";
    char *R2 = "_R2";
    char *extension = ".fastq.gz";

    static struct option lopts[] = {
        {"SE",          0, NULL,   1},
        {"R1",          1, NULL,   2},
        {"R2",          1, NULL,   3},
        {"extension",   1, NULL,   4},
        {"pigzThreads", 1, NULL,   5},
        {"help",        0, NULL, 'h'},
        {0,             0, NULL,   0}
    };
    while((c = getopt_long(argc, argv, "h", lopts, NULL)) >= 0) {
        switch(c) {
        case 'h':
            usage(argv[0]);
            return 0;
        case 1:
            PE = 0;
            break;
        case 2:
            R1 = optarg;
            break;
        case 3:
            R2 = optarg;
            break;
        case 4:
            extension = optarg;
            break;
        case 5:
            pigz_threads = optarg;
            break;
        default:
            fprintf(stderr, "Invalid option '%c'\n", c);
            usage(argv[0]);
            return 1;
        }
    }

    if(argc == 1) {
        usage(argv[0]);
        return 0;
    }

    if(argc - optind < 2) {
        fprintf(stderr, "ERROR: Not enough arguments!\n");
        usage(argv[0]);
        return 1;
    }

    // input and basename
    input = argv[optind];
    bname = argv[optind + 1];

    cmd = calloc(strlen(input) + strlen("zcat  "), sizeof(char));
    assert(cmd);
    sprintf(cmd, "zcat %s", input);
    ifile = popen(cmd, "r");

    if(PE==1) rv = processPaired(ifile, bname, R1, R2, extension, &total, &dupes, pigz_threads);
    else rv = processSingle(ifile, R1, extension, bname, &total, &dupes, pigz_threads);
    pclose(ifile);

    if(rv) {
        fprintf(stderr, "We encountered some sort of error!\n");
        return 1;
    }

    //Output the metrics to a text file
    cmd = calloc(strlen(bname) + strlen(".metrics "), sizeof(char));
    sprintf(cmd, "%s.metrics", bname);
    ifile = fopen(cmd, "w");
    free(cmd);
    fprintf(ifile, "%"PRIu64"\t%"PRIu64"\n", dupes, total);
    fclose(ifile);

    return 0;
}
