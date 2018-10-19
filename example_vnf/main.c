/*
  Simple udp server
*/
#include <stdio.h> // printf
#include <string.h> // memset
#include <stdlib.h> // exit(0);
#include <unistd.h> // close
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/timerfd.h>
#include <sched.h>
#include <math.h>
//#include "newhash.h"

#define BUFLEN 512  // Max length of buffer
#define PORT 1234   // The port on which to listen for incoming data
#define TARGET "10.0.1.1" // The target host
//#define NANOSLEEP_OFFSET 55UL // Offset for nanosleep()
#define CHUNK_SIZE 10240000 // Chunk size for efficient write operations

// Initialize some variables
struct timespec start, end, sleep_interval, t1;
//struct timespec ts_usec_sleep, ts_rem_sleep;
uint32_t usec; // = 100UL - NANOSLEEP_OFFSET;
double mean_usec;
uint32_t sleep_time_printable = 0UL;
time_t proc_time;
int flag_sample = 0;
int flag_sample_all = 0;
int log_level = 0;
int signal_counter = 0;
int c;
int flag_flood = 0;
int max_sample_size = 1;
int sample_counter = 0;
int total_sample_counter = 1;
int flag_start = 0;
int flag_end = 0;
int flag_rand = 0;
int flag_write2file = 0;
int load = 0;
int packetsize = 0;
int interval = 0; // TODO: This should be a float to store non integer intervals
uint_fast32_t sleep_time = 0;

// File related stuff
char file_buffer[CHUNK_SIZE + 64];
int file_buffer_count = 0;
FILE * f;

void die(char *s) {
  perror(s);
  exit(1);
}

void write2file() {
  file_buffer_count += sprintf(&file_buffer[file_buffer_count], "%d;%d;%d;%d;%d;%d;%d;%ld\n", total_sample_counter++, signal_counter, sample_counter, load, packetsize, interval, sleep_time_printable, proc_time);

  if (file_buffer_count >= CHUNK_SIZE) {
    fwrite(file_buffer, 1, file_buffer_count ,f);
    file_buffer_count = 0;
  }
}

void sig_handler(int signo) {
  if (signo == SIGUSR1) {
    flag_sample = 1;
    signal_counter++;
  } else if (signo == SIGINT) {
    printf("Received SIGINT.\nWriting remaining %d byte to file.\n", file_buffer_count);
    if (file_buffer_count > 0) {
      fwrite(file_buffer, 1, file_buffer_count, f);
      fclose(f);
    }
    exit(0);
  }
  if (log_level >= 2) printf("Cought signal: %d\n", signo);
}

time_t time_diff(struct timespec *begin, struct timespec *now, struct timespec *interval) {
  interval->tv_nsec = now->tv_nsec - begin->tv_nsec;
  if(interval->tv_nsec < 0 ){
    interval->tv_nsec += 1000000000;
    interval->tv_sec = now->tv_sec - begin->tv_sec - 1;
  }
  else{
    interval->tv_sec = now->tv_sec - begin->tv_sec;
  }
  long res = interval->tv_nsec + interval->tv_sec * 1E9;
  return(res);
}

double randn(double mu, double sigma) {
  double U1, U2, W, mult;
  static double X1, X2;
  static int call = 0;

  if (call == 1) {
    call = !call;
    return (mu + sigma * (double) X2);
  }

  do {
    U1 = -1 + ((double) rand () / RAND_MAX) * 2;
    U2 = -1 + ((double) rand () / RAND_MAX) * 2;
    W = pow (U1, 2) + pow (U2, 2);
  } while (W >= 1 || W == 0);

  mult = sqrt ((-2 * log (W)) / W);
  X1 = U1 * mult;
  X2 = U2 * mult;

  call = !call;

  return (mu + sigma * (double) X1);
}

int main(int argc, char **argv) {


  // Register signal handler
  signal(SIGUSR1, sig_handler);
  signal(SIGINT, sig_handler);

  // Get two sockets
  struct sockaddr_in sa_in, sa_out, sa_rem;

  int s_in, s_out, i, slen = sizeof(sa_out) , recv_len;
  char buf[BUFLEN];

  extern char *optarg;
  extern int optind;

  // Create a UDP socket; die if it fails
  if ((s_in = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    die("in socket");

  if ((s_out = socket(AF_INET,SOCK_DGRAM, IPPROTO_UDP)) == -1)
    die("out socket");

  // Zero out the structure; safety first
  // Set parameters
  // Bind it

  // Receiving socket
  memset((char *) &sa_in, 0, sizeof(sa_in));
  sa_in.sin_family = AF_INET;
  sa_in.sin_port = htons(PORT);
  sa_in.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(s_in, (struct sockaddr*) &sa_in, sizeof(sa_in) ) == -1)
    die("bind in");

  // Sending socket
  memset((char *) &sa_out, 0 , sizeof(sa_out));
  sa_out.sin_family = AF_INET;
  sa_out.sin_port = htons(0);
  sa_out.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(s_out, (struct sockaddr*) &sa_out, sizeof(sa_out)) == -1)
    die("bind out");

  // Remote address
  memset((char *) &sa_rem, 0 , sizeof(sa_rem));
  sa_rem.sin_family = AF_INET;
  sa_rem.sin_port = htons(PORT);
  inet_aton(TARGET, &sa_rem.sin_addr);

  if (connect(s_out, (struct sockaddr *) &sa_rem, sizeof(sa_rem)) < 0)
    perror("Connect failed");

  // Parse parameter
  while ((c = getopt(argc, argv, "d:fas:l:w:b:p:i:r:")) != -1) {
    switch(c) {
    case 'd':
      usec = (uint32_t) atoi(optarg);
      sleep_time_printable = atoi(optarg);
      break;
    case 'f':
      flag_flood = 1;
      usec = 0;
      sleep_time_printable = 0;
      break;
    case 'a':
      flag_sample_all = 1;
      break;
    case 's':
      max_sample_size = atoi(optarg);
      break;
    case 'l':
      log_level = atoi(optarg);
      break;
    case 'w':
      flag_write2file = 1;
      f = fopen(optarg, "w");
      break;
    case 'b':
      packetsize = atoi(optarg);
      break;
    case 'p':
      load = atoi(optarg);
      break;
    case 'i':
      interval = atoi(optarg);
      break;
    case 'r':
      flag_rand = atoi(optarg);
      mean_usec = usec;
      break;
    }
  }

  // Keep listening for data
  while(1) {

    // Try to receive some data, this is a blocking call; die if it fails
    if ((recv_len = recvfrom(s_in, buf, BUFLEN, 0, (struct sockaddr *) &sa_out, &slen)) == -1)
      die("recvfrom()");

    clock_gettime(CLOCK_MONOTONIC, &start);
    if (flag_sample == 1 || flag_sample_all == 1) {
      flag_start = 1;
    }

    if (flag_flood == 0) {
      if (flag_rand == 1) { // negexp
	mean_usec = -1. * (double)usec * log((double)rand()/(double)(RAND_MAX));
      } else if (flag_rand == 2) { // uniform
	mean_usec = (double)rand()/(double)(RAND_MAX) * usec * 2;
      } else if (flag_rand == 3) { // norm(mu, mu)
	mean_usec = randn(usec, usec);
      } else if (flag_rand == 4) { // norm(mu, 0.2mu)
	mean_usec = randn(usec, 0.2*usec);
      } else {
	mean_usec = usec;
      }

      while(1) {
	if (clock_gettime(CLOCK_MONOTONIC, &t1)) {
	  continue;
	}

	sleep_time = time_diff(&start, &t1, &sleep_interval) / 1000;


	if (sleep_time >= mean_usec) break;
      }


    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    // Now reply the client with the same data
    if (sendto(s_out, buf, recv_len, 0, (struct sockaddr*) &sa_rem, slen) == -1)
      die("sendto()");

    if (flag_sample == 1 || flag_sample_all == 1) {
      proc_time = time_diff(&start, &end, &sleep_interval);
      flag_end = 1;
    }

    if ((flag_sample == 1 || flag_sample_all == 1) && flag_start == 1 && flag_end == 1 && sample_counter < max_sample_size) {
      if (flag_write2file == 1) {
	write2file();
      } else {
	printf("%d;%d;%d;%d;%d;%d;%d;%ld\n", total_sample_counter++, signal_counter, sample_counter+1, load, packetsize, interval, sleep_time_printable, proc_time);
      }
    }

    if (flag_sample == 1  && sample_counter < max_sample_size && flag_start == 1 && flag_end == 1) {
      sample_counter++;
      flag_start = 0;
      flag_end = 0;
    } else if (flag_sample == 1 && sample_counter < max_sample_size && (flag_start == 0 || flag_end == 0)) {
      sample_counter = 0;
      flag_start = 0;
      flag_end = 0;
    } else if (flag_sample == 1 && sample_counter >= max_sample_size) {
      sample_counter = 0;
      flag_sample = 0;
      flag_start = 0;
      flag_end = 0;
    }
  }

  close(s_in);
  close(s_out);
  return 0;
}
