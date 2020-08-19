/*
** Usage ./boris file m [anneal] [smooth] [skip]
** Not filling any in brackets ([]) will use defaults
** n=N        : problem size and m is the cluster target during set-up
** m=M        : cluster target during set-up
** anneal=A   : whether or not to anneal
** smooth=S   : usually try to put a cliff in the data whenever we do iterative tests
**
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#define CHUNK_SIZE 4096

int n = 20, m = 4, anneal = 1, smooth = 8, skip = 1;

void init(int who[], double p[][n+1])
{
    for (int i = 1; i <= n; i++) who[i] = i;
    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= n; j++) {
            if (p[j][i] != -1) {p[i][j] = p[j][i]; continue;}
            int kmax = 5;
            if (i % m == j % m) kmax *= 3;
            if (abs(i%m - j%m) < m/2) kmax += abs(i%m - j%m);
            for (int k = 1; k <= kmax; k++) {
                if (p[i][j] == -1) p[i][j] = 0;
                p[i][j] += drand48();
            }
        }
    }
}

const char *num2color(double n)
{
    int raw = (int) (n*60.0+0.5);
    if (raw < 0) raw = 0;
    if (raw > 255) raw = 255;
    int first = raw/16;
    int second = raw % 16;
    const char key[] = "0123456789ABCDEF";
    char *dest = malloc(7 * sizeof(char));
    dest[0] = key[first];
    dest[1] = key[second];
    dest[2] = key[first];
    dest[3] = key[second];
    dest[4] = key[first];
    dest[5] = key[second];
    dest[6] = '\0';
    return dest;
}

void display(int who[], double arr[][n+1])
{
    printf("<table border=1 cellpadding=0 cellspacing=0>");
    printf("<tr><td></td>");
    for (int j = 1; j <= n; j++)
        printf("<td>%d</td>", who[j]);
    printf("</tr>");
    for (int i = 1; i <= n; i++) {
        printf("<tr><td>%d</td>", who[i]);
        char *color;
        for (int j = 1; j <= n; j++) {
            color = (char *) num2color(arr[i][j]);
            printf("<td bgcolor=%s width=10 height=5>&nbsp;</td>", color);
            free(color);
        }
        printf("</tr>");
    }
    printf("</table>");
}

double score(double temp[][n + 1])
{
    double res = 0;
    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= n; j++) {
            res -= temp[i][j]/(.1+abs(i-j));
        }
    }
    return 1000 - res;
}

void swap(int i, int j, double p[][n + 1], double temp[][n + 1])
{
    int key[n + 1];
    for (int ii = 1; ii <= n; ii++) key[ii] = ii;
    key[i] = j;
    key[j] = i;
    for (int ii = 1; ii <= n; ii++) {
        for (int jj = 1; jj <= n; jj++) {
            temp[ii][jj] = p[key[ii]][key[jj]];
        }
    }
}

int main(int argc, char **argv)
{
    if (argc <= 2) {
        fprintf(stderr, "No file and nspecified");
        return 1;
    }
    char *endptr;
    FILE *f;
    for (int i = 1; i < argc; i++) {
        int tmp = strtol(argv[i], &endptr, 10);
        if (i == 1 && (f = fopen(argv[i], "r")) == NULL) {
            fprintf(stderr, "Invalid File: %s", argv[i]);
            return 1;
        }

        else if (i == 2 && !(n = tmp) && endptr == argv[i]) {
            fprintf(stderr, "Invalid N: %s", argv[i]);
            return 1;
        }

        else if (i == 3 && !(m = tmp) && endptr == argv[i]) {
            fprintf(stderr, "Invalid M: %s", argv[i]);
            return 1;
        }

        else if (i == 4 && !(anneal = tmp) && endptr == argv[i]) {
            fprintf(stderr, "Invalid Anneal: %s", argv[i]);
            return 1;
        }

        else if (i == 5 && !(smooth = tmp) && endptr == argv[i]) {
            fprintf(stderr, "Invalid Smooth: %s", argv[i]);
            return 1;
        }

        else if (i == 6 && !(skip = tmp) && endptr == argv[i]) {

            fprintf(stderr, "Invalid Skip: %s", argv[i]);
            return 1;
        }
    }


    srand48(time(0));
    int who[n + 1];
    double p[n + 1][n + 1];
    for (int i = 0; i <= n; i++)
        for (int j = 0; j <= n; j++)
            p[i][j] = -1;
    init(who, p);

    char *line = NULL;
    ssize_t read;
    size_t len;
    int i = 1, j = 1;
    while ((read = getline(&line, &len, f)) != -1) {
        j = 1;
        char *token = strtok(line, " ");
        while (token != NULL && i <= n && j <= n) {
            double val;
            sscanf(token, "%lf", &val);
            p[i][j] = val;
            token = strtok(NULL, " ");
            j++;
        }
        i++;
    }
    fclose(f);


    printf("Content-type: text/html\n");
    printf("<style>\n");
    printf("td {font-size:8pt}\n");
    printf("</style>\n");


    printf("classes = %d", m);
    display(who, p);
    for (int ep = 1; ep <= 100; ep++) {
        printf("\tepoch %d ", ep);
        int randstep, ibest, jbest;
        double lastBest, best;
        double temp[n+1][n+1];
        if (anneal && drand48() < exp(-(double)ep/(double)smooth)) {
            randstep = 1;
            ibest = jbest = (int) (1. + drand48()*n);
            while (ibest == jbest) jbest = (int) (1. + drand48()*n);
            printf("<font color=red>taking random step </font>");
        } else {
            randstep = 0;
            ibest = jbest = (int) (1 + drand48()*n);
            while (ibest == jbest) jbest = (int) (1 + drand48()*n);
            best = 0;
            for (int i = 1; i <= n; i++) {
                for (int j = i + 1; j <= n; j++) {
                    if (skip && drand48() < exp(-(double)ep/(double)smooth)) {
                        printf("<font color=red>(skipping %d, %d) </font>", i, j);
                        continue;
                    }
                    swap(i, j, p, temp);
                    double ts = score(temp);

                    if (best == 0 || ts > best) {
                        best = ts;
                        ibest = i;
                        jbest = j;
                    }
                }
            }
            if (!anneal && !skip && best < lastBest) return 0;
            lastBest = best;
        }
        swap(ibest, jbest, p, temp);
        for (int i = 1; i <= n; i++)
            for (int j = 1; j <= n; j++)
                p[i][j] = temp[i][j];
        if (randstep) {
            lastBest = best = score(temp);
            printf("random swap is %d %d ", who[ibest], who[jbest]);
            printf("score is %f ", best);
        } else {
            printf("random swap is %d %d ", who[ibest], who[jbest]);
            printf("score is %f ", best);
        }

        int tempwho = who[ibest];
        who[ibest] = who[jbest];
        who[jbest] = tempwho;
        display(who, p);
    }

    return 0;
}
