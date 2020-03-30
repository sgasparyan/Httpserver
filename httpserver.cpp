#include <arpa/inet.h>
#include <cstdint>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <bits/stdc++.h>
#include <string>
#include <list>
#include <iterator>

#define MaxHeader 4000

int validFileName(char *fileName);

using namespace std;

uint8_t logFlag = 0;
uint8_t cacheFlag = 0;
int32_t logFile;


int main(int argc, char **argv) {

  int16_t server_fd, new_socket;
  struct sockaddr_in address;
  int32_t opt = 1;
  int32_t addrlen = sizeof(address);
  char *buffer = (char *)malloc(sizeof(char));
  char *buf = (char *)malloc(sizeof(char) * MaxHeader);

  // socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
  address.sin_family = AF_INET;

  int32_t op;
  char *e;
  uint8_t g;

  if (argc == 1) {
    perror("No Host Specified");
    exit(EXIT_FAILURE);
  } else if (argc == 2) {
    if (strcmp("localhost", argv[1]) == 0) {
      address.sin_addr.s_addr = inet_addr("127.0.0.1");
    } else {
      address.sin_addr.s_addr = inet_addr(argv[1]);
    }
    address.sin_port = htons(80);
  } else if (argc == 3) {

    while ((op = getopt(argc, argv, "c:")) != -1) {
      switch (op) {

      case 'c':
	e = argv[optind];
        cacheFlag = 1;
        break;
      default:
        break;
      }
    }
    if(cacheFlag == 1){
        g = 1;
	while (g < argc) {
            if ((strcmp(argv[g], "-c") != 0)){
                if (strcmp("localhost", argv[g]) == 0) {
                    address.sin_addr.s_addr = inet_addr("127.0.0.1");
                } else {
                    address.sin_addr.s_addr = inet_addr(argv[g]);
                }
                address.sin_port = htons(80);
		break;
	    }
	    g++;
        }
    } else {
        if (strcmp("localhost", argv[1]) == 0) {
            address.sin_addr.s_addr = inet_addr("127.0.0.1");
        } else {
            address.sin_addr.s_addr = inet_addr(argv[1]);
        }
        address.sin_port = htons(atoi(argv[2]));
    }
  } else if (4 <= argc && argc <= 6) {
      g = 1;
      uint8_t y = 0;
      while (g < argc) {
          if (( strcmp(argv[g], "-c") == 0)){
              g++;
	      cacheFlag = 1;
	  } else if ((strcmp(argv[g], "-l") == 0)){
	      logFlag = 1;
	      logFile = open(argv[g+1], O_WRONLY | O_TRUNC);
	      if (logFile == -1) {
                  logFile = open(argv[g+1], O_WRONLY | O_CREAT, S_IRWXU);
	      }
	      g += 2;
	  } else {
              if (y == 0) {
                  if (strcmp("localhost", argv[g]) == 0) {
                      address.sin_addr.s_addr = inet_addr("127.0.0.1");
                  } else {
                      address.sin_addr.s_addr = inet_addr(argv[g]);
                  }
                  address.sin_port = htons(80);
              } else if (y == 1) {
                  address.sin_port = htons(atoi(argv[g]));
              }
              g++;
              y++;
	  }
      }
  } else {
    perror("Too Many Arguments");
    exit(EXIT_FAILURE);
  }
  // attach socket to the port
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
  if (listen(server_fd, 16) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  vector <string> fileNames;
  vector <string> cache;
  vector <int> dirty;

  while (1) {

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0) {
      perror("accept");
      exit(EXIT_FAILURE);
    }
    uint32_t i = 0;
    while (read(new_socket, buffer, 1) > 0) {
      buf[i] = *buffer;
      if (i > 2) {
        if (buf[i - 3] == '\r' && buf[i - 2] == '\n' && buf[i - 1] == '\r' &&
            buf[i] == '\n') {
          break;
        }
      }
      i++;
    }
    char *fileName;
    char *tok;
    int8_t fd;
    char *protocol = strtok(buf, " ");
    uint8_t flag = -1;
    uint32_t count = 0;
    uint8_t size;
    char* tempE = NULL;
    uint8_t exitFlag = 0;
    

    if (strcmp(protocol, "GET") == 0) {

      fileName = strtok(NULL, " /");
      if (validFileName(fileName) == 0) {
          dprintf(new_socket, "HTTP/1.1 400 Bad Request\r\n");
	  dprintf(new_socket, "Content-Length: 0\r\n\r\n");
          if (logFlag == 1) {
              size = 46 + strlen(fileName);
              tempE = (char *)malloc(size * sizeof(char));
              sprintf(tempE, "FAIL: GET %s HTTP/1.1 --- response 400\n========\n", fileName);
              write(logFile, tempE, size);
	  }
          close(new_socket);
          continue;
      }
      if (cacheFlag == 1){
          uint8_t p = 0;
	  while (p < fileNames.size()){

              if (fileNames[p].compare(fileName) == 0){
                  
                  uint32_t sizeOfFile = (uint32_t) cache[p].length();
		  dprintf(new_socket, "HTTP/1.1 200 OK\r\n");
                  dprintf(new_socket, "Content-Length: %d\r\n\r\n", sizeOfFile);
		  char* cacheBuf = (char*) calloc(1, sizeof(char));
		  for(uint32_t z = 0; z < sizeOfFile;z++){
                      cacheBuf = &(cache[p][z]);
		      write(new_socket, cacheBuf, 1);
	          }
		  
		  exitFlag = 1;
		  if (logFlag == 1){
                      size = 65;
                      tempE = (char*) malloc(size * sizeof(char));
                      sprintf(tempE, "GET %s length 0 [was in cache]\n========\n", fileName);
                      write(logFile, tempE, size);
		  }
		  close(new_socket);
	          break;
	      }
	      p++;
          }
	  if (exitFlag == 1){
              continue;
	  }
      }
      fd = open(fileName, O_RDONLY);
      if (fd == -1) {
        if (errno == 2) {
          dprintf(new_socket, "HTTP/1.1 404 Not Found\r\n");
	  dprintf(new_socket, "Content-Length: 0\r\n\r\n");
	  if (logFlag == 1) {
	      size = 46 + strlen(fileName);
              tempE = (char *)malloc(size * sizeof(char));
              sprintf(tempE, "FAIL: GET %s HTTP/1.1 --- response 404\n========\n", fileName);
	      write(logFile, tempE, size); 
	  }
          close(new_socket);
          continue;
        } else if (errno == EACCES) {
          dprintf(new_socket, "HTTP/1.1 403 Forbidden\r\n");
	  dprintf(new_socket, "Content-Length: 0\r\n\r\n");
	  if (logFlag == 1) {
              size = 46 + strlen(fileName);
              tempE = (char *)malloc(size * sizeof(char));
              sprintf(tempE, "FAIL: GET %s HTTP/1.1 --- response 403\n========\n", fileName);
              write(logFile, tempE, size);
	  }
          close(new_socket);
          continue;
        }
      }
      i = 0;
      while (read(fd, buffer, 1) > 0) {
        i++;
      }
      close(fd);
      dprintf(new_socket, "HTTP/1.1 200 OK\r\n");
      dprintf(new_socket, "Content-Length: %d\r\n\r\n", i);
      fd = open(fileName, O_RDONLY);
      string ch = "";
      while (read(fd, buffer, 1) > 0) {
        write(new_socket, buffer, 1);
	ch += *buffer;
      }
      if (logFlag == 1) {
	  size = 69;
          tempE = (char*) malloc(size * sizeof(char));
          sprintf(tempE, "GET %s length 0 [was not in cache]\n========\n", fileName);
          write(logFile, tempE, size);
      }
      if (cacheFlag == 1){
          if (fileNames.size() == 4){
	      char* fn = (char*) calloc(27, sizeof(char));
              char b;
              string f = fileNames[0];
              string c = cache[0];
	      int dirt = dirty[0];
              if (dirt == 1){
                  for(uint8_t in = 0; in < f.length(); in++){
                      fn[in] = f[in];
                  }
                  fd = open(fn, O_WRONLY | O_TRUNC);
	          if (fd == -1) {
                      fd = open(fn, O_WRONLY | O_CREAT, S_IRWXU);
                  }
                  for(uint32_t in = 0; in < c.length();in++){
                      b = c[in];
                      write(fd, &b, 1);
                  }
              }
              fileNames.erase(fileNames.begin());
	      cache.erase(cache.begin());
	      dirty.erase(dirty.begin());
          }
          fileNames.push_back(string(fileName));
          cache.push_back(ch);
	  dirty.push_back(0);
      }
      free(tempE);
    } else if (strcmp(protocol, "PUT") == 0) {
      fileName = strtok(NULL, " /");

      if (validFileName(fileName) == 0) {
        dprintf(new_socket, "HTTP/1.1 400 Bad Request\r\n");
	dprintf(new_socket, "Content-Length: 0\r\n\r\n");
	if (logFlag == 1){
            size = 46 + strlen(fileName);
            tempE = (char *)malloc(size * sizeof(char));
            sprintf(tempE, "FAIL: PUT %s HTTP/1.1 --- response 400\n========\n", fileName);
            write(logFile, tempE, size);
	}
        close(new_socket);
        continue;
      }
      count = 0;
      char* bob;
      while (count != 8) {
          bob = strtok(NULL, " \r\n");
          count++;
      }
      tok = strtok(NULL, " \r\n");
      count = atoi(tok);
      uint8_t rem; 
      if (cacheFlag == 1){
          for (uint8_t p = 0; p < fileNames.size();p++){
              if (fileNames[p].compare(fileName) == 0){
		  cache[p] = "";
		  uint32_t ff = 0;
                  while (read(new_socket, buffer, 1) > 0) {
                      cache[p] += *buffer;
		      ff++;
		      if (ff == count){
                          break;
		      }
                  }
                  exitFlag = 1;
		  rem = p;
		  dirty[p] = 1;
                  break;
              }
          }
          if (exitFlag == 1){
              dprintf(new_socket, "HTTP/1.1 200 OK\r\n");
              dprintf(new_socket, "Content-Length: 0\r\n\r\n");
	      if (logFlag == 1){
		  i = 0;
                  uint8_t j = 0;
                  int leb = 0;
                  char* bigBuf = (char*)malloc(20 * sizeof(char));
		  size = 55 + strlen(tok);
                  tempE = (char*) malloc(size * sizeof(char));
                  sprintf(tempE, "PUT %s length %s [was in cache]\n", fileName, tok);
                  write(logFile, tempE, size);
		  for(uint32_t q = 0; q < cache[rem].length();q++){
                      i++;
                      bigBuf[j] = cache[rem][q];
                      j = i % 20; 
                      if (j == 0) {
                          size = 8;
                          tempE = (char*)malloc(size * sizeof(char));
                          sprintf(tempE, "%08d", leb);
                          write(logFile, tempE, size);
                          leb += 20;
                          for (uint8_t y = 0; y < 20; y++){
                              size = 3;
                              tempE = (char*)malloc(size * sizeof(char));
                              sprintf(tempE, " %02x", (unsigned char)bigBuf[y]);
                              write(logFile, tempE, size);
                          }
                          write(logFile, "\n", 1);
                      }
                  
                      if (count == i) {
                          if (logFlag == 1) {
                              if (j == 0) {
                                  size = 9;
                                  tempE = (char*) malloc(size * sizeof(char));
                                  sprintf(tempE, "=======\n");
                                  write(logFile, tempE, size);
                              } else {
                                  size = 8;
                                  tempE = (char*)malloc(size * sizeof(char));
                                  sprintf(tempE, "%08d", leb);
                                  write(logFile, tempE, size);

                                  for (uint8_t x = 0; x < j;x++){
                                      size = 3;
                                      tempE = (char*)malloc(size * sizeof(char));
                                      sprintf(tempE, " %02x", (unsigned char) bigBuf[x]);
                                      write(logFile, tempE, size);
                                  }
                                  size = 10;
                                  tempE = (char*)malloc(size * sizeof(char));
                                  sprintf(tempE, "\n========\n");
                                  write(logFile, tempE, size);
                              }
                          }
                          free(tempE);
                          free(bigBuf);
                          break;
                      }
		  }
	      }
	      close(new_socket);
	      continue;
          }
      }
      if (cacheFlag == 0){
      fd = open(fileName, O_WRONLY | O_TRUNC);
       
      if (fd == -1) {
        fd = open(fileName, O_WRONLY | O_CREAT, S_IRWXU);
        flag = 1;
      } else {
        flag = 0;
        if (errno == EACCES) {
          dprintf(new_socket, "HTTP/1.1 403 Forbidden\r\n");
	  dprintf(new_socket, "Content-Length: 0\r\n\r\n");
          if (logFlag == 1){
              size = 46 + strlen(fileName);
              tempE = (char *)malloc(size * sizeof(char));
              sprintf(tempE, "FAIL: PUT %s HTTP/1.1 --- response 403\n========\n", fileName);
              write(logFile, tempE, size);
          }
          close(new_socket);
          continue;
        }
      }
      }
      if (strcmp(bob, "Content-Length:") == 0) {
        count = atoi(tok);
        if (logFlag == 1){
            size = 59 + strlen(tok);
	    tempE = (char*) malloc(size * sizeof(char));
	    sprintf(tempE, "PUT %s length %s [was not in cache]\n", fileName, tok);
	    write(logFile, tempE, size);
	}

        i = 0;
	uint8_t j = 0;
	int leb = 0;
	char* bigBuf = (char*)malloc(20 * sizeof(char));
	string ch;
        while (read(new_socket, buffer, 1) > 0) {
	  i++;
	  ch += *buffer;
	  bigBuf[j] = *buffer;
	  j = i % 20;
          if (j == 0) {
              if (logFlag == 1){
                  size = 8;
                  tempE = (char*)malloc(size * sizeof(char));
                  sprintf(tempE, "%08d", leb);
                  write(logFile, tempE, size);
		  leb += 20;
		  for (uint8_t y = 0; y < 20; y++){
                      size = 3;
		      tempE = (char*)malloc(size * sizeof(char));
		      sprintf(tempE, " %02x", (unsigned char)bigBuf[y]);
                      write(logFile, tempE, size);
		  }
		  write(logFile, "\n", 1);
	      }
          } 
	  if (cacheFlag == 0){
              write(fd, buffer, 1);
	  }
          if (count == i) {
              if (logFlag == 1) {
                  if (j == 0) {
                      size = 9;
		      tempE = (char*) malloc(size * sizeof(char));
		      sprintf(tempE, "=======\n");
		      write(logFile, tempE, size);
		  } else {
                      size = 8;
		      tempE = (char*)malloc(size * sizeof(char));
		      sprintf(tempE, "%08d", leb);
		      write(logFile, tempE, size);

		      for (uint8_t x = 0; x < j;x++){
                          size = 3;
			  tempE = (char*)malloc(size * sizeof(char));
			  sprintf(tempE, " %02x", (unsigned char) bigBuf[x]);
			  write(logFile, tempE, size);
		      }
                      size = 10;
		      tempE = (char*)malloc(size * sizeof(char));
		      sprintf(tempE, "\n========\n");
		      write(logFile, tempE, size); 
		  }
	      }
	      free(tempE);
	      free(bigBuf);
              break;
          }
        }
        if (flag == 0) {
          dprintf(new_socket, "HTTP/1.1 200 OK\r\n");
          dprintf(new_socket, "Content-Length: 0\r\n\r\n");
        } else if (flag == 1) {
          dprintf(new_socket, "HTTP/1.1 201 Created\r\n");
          dprintf(new_socket, "Content-Length: 0\r\n\r\n");
        }

	  if (cacheFlag == 1){
              if (fileNames.size() == 4){

		  char* fn = (char*) calloc(27, sizeof(char));;
		  char b;
		  string f = fileNames[0];
		  string c = cache[0];
		  int dirt = dirty[0];
		  if (dirt == 1){
		      for(uint8_t in = 0; in < f.length(); in++){
                          fn[in] = f[in];
	              }
		      fd = open(fn, O_WRONLY | O_TRUNC);
		      if (fd == -1) {
                          fd = open(fn, O_WRONLY | O_CREAT, S_IRWXU);
                      }
		      for(uint32_t in = 0; in < c.length();in++){
                          b = c[in];
		          write(fd, &b, 1);
	              } 
		  }
                  fileNames.erase(fileNames.begin());
                  cache.erase(cache.begin());
		  dirty.erase(dirty.begin());
              }

              fileNames.push_back(string(fileName));
              cache.push_back(ch);
	      dirty.push_back(1);
          }

      }
    } else {
      fileName = strtok(NULL, " /");
      dprintf(new_socket, "HTTP/1.1 500 Internal Service Error\r\n");
      dprintf(new_socket, "Content-Length: 0\r\n\r\n");
      if (logFlag == 1){
          size = 43 + strlen(fileName) + strlen(protocol);
	  tempE = (char*)malloc(size*sizeof(char));
	  sprintf(tempE, "FAIL: %s %s HTTP/1.1 --- response 500\n========\n",protocol, fileName);
          write(logFile, tempE, size);
      }
      close(new_socket);
      continue;
    }
    close(new_socket);
  }
  free(buf);
  free(buffer);
  return 0;
}

int validFileName(char *fileName) {
  char c = *fileName;
  if (strlen(fileName) != 27) {
    return 0;
  }
  while (c != '\0') {

    c = *fileName;
    if ('a' <= c && c <= 'z') {
      fileName++;
      c = *fileName;
    } else if ('A' <= c && c <= 'Z') {
      fileName++;
      c = *fileName;
    } else if ('0' <= c && c <= '9') {
      fileName++;
      c = *fileName;
    } else if (c == '-' || c == '_') {
      fileName++;
      c = *fileName;
    } else{
      return 0;
    }
  }
  return 1;
}
