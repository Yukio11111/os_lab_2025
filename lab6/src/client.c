#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdint.h>
#include <inttypes.h>

struct Server {
  char ip[255];
  int port;
};

struct ClientThreadArgs {
  struct Server *server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t *result;
};

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
  uint64_t result = 0;
  a = a % mod;
  while (b > 0) {
    if (b % 2 == 1)
      result = (result + a) % mod;
    a = (a * 2) % mod;
    b /= 2;
  }
  return result % mod;
}

bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }
  if (errno != 0)
    return false;
  *val = i;
  return true;
}

void *ClientThread(void *arg) {
  struct ClientThreadArgs *args = (struct ClientThreadArgs *)arg;
  struct hostent *hostname = gethostbyname(args->server->ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", args->server->ip);
    *args->result = 1; // Default to 1 on error
    return NULL;
  }
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(args->server->port);
  memcpy(&server_addr.sin_addr, hostname->h_addr_list[0], sizeof(struct in_addr));
  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    *args->result = 1;
    return NULL;
  }
  if (connect(sck, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    fprintf(stderr, "Connection failed\n");
    *args->result = 1;
    close(sck);
    return NULL;
  }
  char task[sizeof(uint64_t) * 3];
  memcpy(task, &args->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &args->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &args->mod, sizeof(uint64_t));
  if (send(sck, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send failed\n");
    *args->result = 1;
    close(sck);
    return NULL;
  }
  char response[sizeof(uint64_t)];
  if (recv(sck, response, sizeof(response), 0) < 0) {
    fprintf(stderr, "Receive failed\n");
    *args->result = 1;
    close(sck);
    return NULL;
  }
  memcpy(args->result, response, sizeof(uint64_t));
  close(sck);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = 0; // Changed to 0 for check
  uint64_t mod = 0;
  char servers_path[255] = {'\0'}; // 255 - arbitrary limit for file path, enough for most cases (PATH_MAX is 4096, but simple)
  while (true) {
    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};
    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1)
      break;
    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        if (!ConvertStringToUI64(optarg, &k)) {
          return 1;
        }
        break;
      case 1:
        if (!ConvertStringToUI64(optarg, &mod)) {
          return 1;
        }
        break;
      case 2:
        strncpy(servers_path, optarg, 254);
        servers_path[254] = '\0';
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;
    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }
  if (k == 0 || mod == 0 || !strlen(servers_path)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n", argv[0]);
    return 1;
  }
  // Read servers from file
  FILE *fp = fopen(servers_path, "r");
  if (!fp) {
    fprintf(stderr, "Cannot open servers file: %s\n", servers_path);
    return 1;
  }
  unsigned int servers_num = 0;
  char line[256];
  while (fgets(line, sizeof(line), fp)) {
    if (line[0] != '\n' && line[0] != '#') servers_num++;
  }
  rewind(fp);
  if (servers_num == 0) {
    fprintf(stderr, "No servers in file\n");
    fclose(fp);
    return 1;
  }
  struct Server *to = malloc(sizeof(struct Server) * servers_num);
  int idx = 0;
  while (fgets(line, sizeof(line), fp)) {
    if (line[0] == '\n' || line[0] == '#') continue;
    char *colon = strchr(line, ':');
    if (!colon) {
      fprintf(stderr, "Invalid line: %s", line);
      continue;
    }
    *colon = '\0';
    char *port_str = colon + 1;
    // Remove newline
    char *nl = strchr(port_str, '\n');
    if (nl) *nl = '\0';
    strncpy(to[idx].ip, line, 254);
    to[idx].ip[254] = '\0';
    to[idx].port = atoi(port_str);
    if (to[idx].port <= 0) {
      fprintf(stderr, "Invalid port in line: %s\n", line);
      continue;
    }
    idx++;
  }
  fclose(fp);
  // Parallel work
  uint64_t *partials = malloc(sizeof(uint64_t) * servers_num);
  pthread_t threads[servers_num];
  struct ClientThreadArgs *thread_args[servers_num];
  uint64_t range = k;
  uint64_t part = range / servers_num;
  uint64_t rem = range % servers_num;
  uint64_t current = 1;
  for (unsigned int i = 0; i < servers_num; i++) {
    uint64_t extra = ((uint64_t)i < rem) ? 1 : 0;
    uint64_t b = current;
    uint64_t e = current + part + extra - 1;
    if (e > k) e = k;
    thread_args[i] = malloc(sizeof(struct ClientThreadArgs));
    thread_args[i]->server = &to[i];
    thread_args[i]->begin = b;
    thread_args[i]->end = e;
    thread_args[i]->mod = mod;
    thread_args[i]->result = &partials[i];
    if (pthread_create(&threads[i], NULL, ClientThread, thread_args[i])) {
      fprintf(stderr, "pthread_create failed\n");
      return 1;
    }
    current = e + 1;
  }
  for (unsigned int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    free(thread_args[i]);
  }
  // Unite results
  uint64_t answer = 1;
  for (unsigned int i = 0; i < servers_num; i++) {
    answer = MultModulo(answer, partials[i], mod);
  }
  printf("answer: %" PRIu64 "\n", answer);
  free(partials);
  free(to);
  return 0;
}