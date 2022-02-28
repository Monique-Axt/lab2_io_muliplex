#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/fcntl.h>
#include <poll.h>
#include <unordered_map>
#include <map>
#include <iomanip>
#include "Restart.h"
#include "PiCalculator.h"
#include "eCalculator.h"


int nrDecimalPlaces = 50000;
const char *const eFilename = "e.dat";
const char *const piFilename = "pi.dat";
int myPID;

bool cmdLineHandler(int argc, char *argv[]);
void setDefaultValue();
void printOutput(const map<int, int> &map, int count, const char whichCalc[12]);

int main(int argc, char* argv[]) {
    if (!cmdLineHandler(argc, argv))    {
        std::cout << "Using default values." << std::endl;
        setDefaultValue();
    }

    /** create FDs for pipes for IPC */
    int fd_pipe_pi[2];
    int fd_pipe_e[2];

    if (pipe(fd_pipe_pi) == -1 || pipe(fd_pipe_e) == -1){
        std::perror("Failed to create the pipes");
        return 1;
    }

    /** initiate rules for poll */
    struct pollfd *pollfds;
    pollfds[0].fd = *fd_pipe_e;          //file descriptor
    pollfds[0].events = POLLRDNORM;;     //requested events (POLLRDNORM=read normal data w/o blocking)
    pollfds[1].fd = *fd_pipe_pi;
    pollfds[1].events = POLLRDNORM;;


/** open files that parent will write to and associate file-descriptors to the files
 done before forking so that parent and children share the system file table entry */
    int file_e, file_pi;
    mode_t fdmode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if ((file_e = open(eFilename, O_RDWR | O_CREAT | O_NONBLOCK, fdmode)) == -1) {     // open e.dat
        std::perror("Failed to open file");
        return 1;
    }

    if ((file_pi = open(piFilename,  O_RDWR | O_CREAT | O_NONBLOCK, fdmode)) == -1) {   // open pi.dat
        std::perror("Failed to open file");
        return 1;
    }

    /** fork() */
    pid_t pidList[2];
    for(int &retID : pidList) {
        retID = fork();
        myPID = retID;

        if (retID == -1)        {
            std::perror("Failed to fork");
            return 1;
        }

        else if (retID == 0) {// child process
            break;
        }
    }


    /** parent code **************************************/
    if (myPID > 0) {
        /* maps for distribution calculations */
        std::map<int, int> e_map;
        std::map<int, int> pi_map;

        for (auto i = 0; i <= 9; i++){
            e_map[i] = 0;
            pi_map[i] = 0;
        }

        int nrFdEvents;
        int i;
        // int totalCount = 0;
        int eCount = 0;
        int piCount = 0;

        /* time-out poll */
        while (true) {
            nrFdEvents = poll(pollfds, 2, 2000);   // holds the number of descriptors that have events

            if ((nrFdEvents == -1) && (errno == EINTR))
                continue;                                     // poll interrupted by a signal, try again

            else if (nrFdEvents == -1) {                      // poll error
                perror("Poll error:");
                break;
            }
            else if (nrFdEvents == 0) {
                std::cout << std::endl << "Done! (timed out)" << std::endl;
                break;
            }

            // handle polling events
            for (i = 0; i < 2 && nrFdEvents > 0; i++) {
                char outputChar;
                int bytesread;
                int tempReadPipe;

                if ((pollfds + i)->revents) {
                    if ((pollfds + i)->revents & (POLLRDNORM | POLLIN)) {
                        int *tempFile;

                        if (i == 0) {        /** e code */
                            tempReadPipe = fd_pipe_e[0];
                            tempFile = &file_e;

                        } else if (i == 1) {  /** pi code*/
                            tempReadPipe = fd_pipe_pi[0];
                            tempFile = &file_pi;
                        }

                        bytesread = r_read(tempReadPipe, &outputChar, sizeof(outputChar));
                        nrFdEvents--;

                        if (bytesread > 0) {
                            r_write(*tempFile, &outputChar, sizeof(outputChar));
                            //totalCount++;
                            int mapIndex = outputChar - '0';

                            if (i == 0) { /** e code */
                                e_map[mapIndex] += 1;

                                if (eCount++ > 0 && (eCount % 1000) == 0)
                                    printOutput(e_map, eCount, "eCalculator");

                            } else if (i == 1) {  /** pi code*/
                                pi_map[mapIndex] += 1;

                                if (piCount++ > 0 && ((piCount % 1000) == 0))
                                    printOutput(pi_map, piCount, "PiCalculator");
                            }

                        } else {
                            bytesread = -1;
                        }
                    } else if ((pollfds + i)->revents & (POLLERR | POLLHUP)) {  // end of file
                        bytesread = -1;
                    } else {
                        bytesread = 0;    // descriptor not involved in this round
                    }

                    if (bytesread == -1) {
                        if (r_close(tempReadPipe)) {
                            perror("parent can't close pipe");
                        }
                        (pollfds + i)->fd = -1;
                    }
                } //  if ((pollfds+i)->revents)
            } // for-loop end
        } // while end

        std::clog << "Goodbye..." << std::endl;
        return 0;
    } // parent end

        /** children code **************************************/
    else {
        /* e child code **/
        if (myPID == pidList[0]) {
            eCalculator eCalculator(nrDecimalPlaces, fd_pipe_e[1]);
            eCalculator.doCalc();
            return 0;
        }

            /* PI child code */
        else {
            PiCalculator piCalculator(nrDecimalPlaces, fd_pipe_pi[1]);
            piCalculator.doCalc();
            return 0;
        }
    }   // end children code
}   // end main

void printOutput(const map<int, int> &map, int count, const char whichCalc[12]) {
    std::cout << whichCalc << " n = " << count << "/" << nrDecimalPlaces << ":" << std::endl;
    std::cout << "Nr\tCount\tPercentage" << std::endl;

    int totalCount = 0;
    double totalPercentage = 0;

    for (auto &pair : map) {
        double percentage = 0;

        if (pair.second != 0)
            percentage = (pair.second /(double)(count)) * 100.00;

        totalPercentage += percentage;
        totalCount += pair.second;

        std::cout  << pair.first << "\t" << pair.second << "\t% " << percentage  << "\n";
    }

    std::cout << "\t----------------\n"
              << "\t" << totalCount << "\t% " << std::fixed << std::setprecision(2) << totalPercentage  << std::endl;
    std::cout << "\t----------------\n" << std::endl;
}

void setDefaultValue() {
    nrDecimalPlaces = 50000;
}

bool cmdLineHandler(int argc, char *argv[]) {
    if (argc > 1)
        try {
            nrDecimalPlaces = atoi(argv[1]);
        } catch (const std::bad_cast& e) {
            std::cout << "Bad/missing argument. ";
            return false;
        }
    return true;
}