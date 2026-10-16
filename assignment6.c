#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

int main() {
    int n, rounds;

    // Step 1 - get input
    printf("Enter the number of voters: ");
    scanf("%d", &n);
    printf("Enter the number of rounds: ");
    scanf("%d", &rounds);
    if (n < 3) {
        printf("Number of voters must be at least 3.\n");
        return 1;
    }

    // Disable buffering for stdout to ensure immediate output
    setvbuf(stdout, NULL, _IONBF, 0);

    // Step 2 - create pipes for communication between judge and voters
    int fd[n][2];
    for (int i = 0; i < n; i++) {
        // create a pipe for each voter
        if (pipe(fd[i]) < 0) { perror("pipe"); return 1; }
    }

    // Step 3 - judge creates N children. voter_id = -1 for the judge.
    int voter_id = -1;
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); return 1; }
        // In the child process, set voter_id to i and break the loop
        if (pid == 0) {
            voter_id = i;
            break;
        }
    }

    // Step 4 - Judge collects votes from all voters and prints the result
    if (voter_id == -1) {
        // Close unused pipe ends in the judge process
        for (int i = 0; i < n; i++) close(fd[i][1]);

        for (int r = 1; r <= rounds; r++) {
            printf("\n\n------ Round %d -------\n", r);
            int yes = 0;
            // read votes from all voters
            for (int i = 0; i < n; i++) {
                int vote;
                read(fd[i][0], &vote, sizeof(vote));
                yes += vote;
            }
            // print the result of the round
            printf("--- Round %d: The voting result is a %s.\n", r,
                   yes > n / 2 ? "YES" : "NO");
        }
        // wait for all children to finish
        for (int i = 0; i < n; i++) wait(NULL);
    } else {
        // voter process
        for (int i = 0; i < n; i++) {
            // close unused pipe ends
            close(fd[i][0]);
            if (i != voter_id) close(fd[i][1]);
        }
        // seed the random number generator with a combination of time and process ID
        srand(time(NULL) ^ (getpid() << 8));

        for (int r = 1; r <= rounds; r++) {
            // randomly vote YES (1) or NO (0)
            int vote = rand() % 2;
            printf("Round %d: Voter %d votes: %s\n", r, voter_id, vote ? "YES" : "NO");
            write(fd[voter_id][1], &vote, sizeof(vote));
        }
        close(fd[voter_id][1]);
        exit(0);
    }
    return 0;
}