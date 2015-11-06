#include "windows.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> 

HANDLE openFile (char *filename, SECURITY_ATTRIBUTES *sa, int writable)
{
  DWORD access = GENERIC_WRITE,
        creation = CREATE_ALWAYS;
  if (!writable) {
    access = GENERIC_READ;
    creation = OPEN_EXISTING;
  }

  if (filename) {
    HANDLE f = CreateFile(filename,
                      access,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      sa,
                      creation,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL
                      );
    if (f == INVALID_HANDLE_VALUE) {
      printf("Open file %s failed (%d).\n", filename, GetLastError());
      exit(EXIT_FAILURE);
    }
    return f;
  }
  return NULL;
}

void usage()
{
  printf("Usage: \n");
  exit(EXIT_SUCCESS);
}

char *copystr(char *from)
{
  char *to = malloc(strlen(from));
  strcpy(to,from);
  return to;
}

int main(int argc, char *argv[])
{
  char opt;
  char *exefile = NULL,
       *pidfile = NULL,
       *inputfile = NULL,
       *outputfile = NULL,
       *errorfile = NULL,
       *workdir = NULL;

  while ((opt = getopt(argc, argv, "p:i:o:e:hd:")) != -1) {
  switch (opt) {
    case 'p':
      pidfile = copystr(optarg);
      break;
    case 'i':
      inputfile = copystr(optarg);
      break;
    case 'o':
      outputfile = copystr(optarg);
      break;
    case 'e':
      errorfile = copystr(optarg);
      break;
    case 'd':
      workdir = copystr(optarg);
      break;
    case 'h':
    default:
      usage();
      break;
    }
  }

  if (optind >= argc) {
    usage();
  }

  exefile = copystr(argv[optind]);
  
  /*
   * почему-то не работает
   *
  if ( ! SetHandleInformation(GetStdHandle(STD_INPUT_HANDLE), HANDLE_FLAG_INHERIT, 0) ) {
    printf( "Stdin SetHandleInformation filed (%d).\n", GetLastError() );
    exit(EXIT_FAILURE);
  }
  if ( ! SetHandleInformation(GetStdHandle(STD_OUTPUT_HANDLE), HANDLE_FLAG_INHERIT, 0) ) {
    printf( "Stdout SetHandleInformation filed (%d).\n", GetLastError() );
    exit(EXIT_FAILURE);
  }
  if ( ! SetHandleInformation(GetStdHandle(STD_ERROR_HANDLE), HANDLE_FLAG_INHERIT, 0) ) {
    printf( "Stderr SetHandleInformation filed (%d).\n", GetLastError() );
    exit(EXIT_FAILURE);
  }*/

  STARTUPINFO si;
  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
  si.wShowWindow = SW_HIDE;

  SECURITY_ATTRIBUTES sa;
  sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle       = TRUE;

  si.hStdInput  = openFile(inputfile, &sa, FALSE);
  si.hStdOutput = openFile(outputfile, &sa, TRUE);
  si.hStdError  = openFile(errorfile, &sa, TRUE);

  // костыль, но зато нормально отцепляется процесс от php-cgi
  CloseHandle(GetStdHandle(STD_INPUT_HANDLE));
  CloseHandle(GetStdHandle(STD_OUTPUT_HANDLE));
  CloseHandle(GetStdHandle(STD_ERROR_HANDLE));

  PROCESS_INFORMATION pi;
  if (!CreateProcess(NULL,
                     exefile,
                     NULL,
                     NULL,
                     TRUE,
                     DETACHED_PROCESS | CREATE_UNICODE_ENVIRONMENT,
                     NULL,
                     workdir,
                     &si,
                     &pi
                     ))
  {
    printf( "Create process failed (%d).\n", GetLastError() );
    exit(EXIT_FAILURE);
  };

  if (pidfile) {
    char pidBuffer[8];
    while (!pi.dwProcessId){ //почему-то pid доступен бывает не сразу
      sleep (1);
    }
    //TODO: bug in itoa?
    itoa(pi.dwProcessId,pidBuffer,10);
    DWORD dwBytesWritten = 0;
    HANDLE f = CreateFile(pidfile,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    WriteFile(f, pidBuffer, 8, &dwBytesWritten, NULL);
    CloseHandle(f);
  }

  //WaitForSingleObject (pi.hProcess, INFINITE);
  //TerminateProcess(pi.hProcess,NO_ERROR);

  return 0;
}