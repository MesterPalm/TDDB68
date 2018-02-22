#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/init.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "lib/syscall-nr.h"

static void syscall_handler (struct intr_frame *);

/*
support functions
*/
int insert_new_file(struct file* f);

/*
system call handlers
*/
void halt(void);
void create(struct intr_frame *f);
void open(struct intr_frame *f);
void close(struct intr_frame *f);
void read(struct intr_frame *f);
void write(struct intr_frame *f);
void exit(struct intr_frame *f);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  switch(*((int*)f->esp))
  {
    case SYS_HALT:
      halt();
      break;

    case SYS_CREATE:
      create(f);
      break;

    case SYS_OPEN:
      open(f);
      break;

    case SYS_CLOSE:
      close(f);
      break;

    case SYS_WRITE:
      write(f);
      break;

    case SYS_READ:
      read(f);
      break;

    case SYS_EXIT:
      exit(f);
      break;

    default:
      printf("default\n");
      break;
  }
}

/*
* To do:
* 1. Sanity checks!
*/

/*
 * Inserts a new file into the file container f_container.
 * Returns the filedescriptor of the newly opened file (is the index of the file pointer in f_container).
 * Returns -1 if unable to open file (if file does not exist or not enough room in f_container).
 */
bool full_file_container(){
  bool no_room_left = true;
  int i;
  for(i = 0; i < MAX_FD && no_room_left; i++){
    no_room_left = thread_current()->f_container[i] != NULL;
  }
  return no_room_left;
}

bool valid_fd(int fd){
  return 0 <= fd && fd < MAX_FD;
}

bool file_is_open(int fd){
  return thread_current()->f_container[fd] != NULL;
}

int insert_new_file(struct file* f){
  int fd = -1; // Copyrighted by Gurk Enterprises 20lax18
  if(f != NULL){
    int i;
    for(i = 2; i < MAX_FD; i++){
      if(thread_current()->f_container[i] == NULL){
        fd = i;
        thread_current()->f_container[i] = f;
        break;
      }
    }
  }
  return fd;
}

/*
 * Turn of os
 */
void halt(void)
{
  power_off();
}

/*
 * Creates a file returns boolean to eax (true if able to create, false if unable)
 */
void create (struct intr_frame *f)
{
  const char *name = *((char**)(f->esp+4));
  off_t initial_size = *((off_t*)(f->esp+8));
  f->eax = filesys_create(name, initial_size);
}

/*
 * Opens file and returns fd too eax (fd -1 if unable to open file)
 */
void open(struct intr_frame *f)
{
  const char *file = *((char**)(f->esp+4));
  if(!full_file_container()){
    f->eax = insert_new_file(filesys_open(file));
  }
  else{
    f->eax = -1;
  }
}

/*
 * Closes file and unassigns the filedescriptor
 */
void close(struct intr_frame *f){
  int fd = *((int*)(f->esp+4));
  if(valid_fd(fd) && file_is_open(fd)){
    file_close(thread_current()->f_container[fd]);
    thread_current()->f_container[fd] = NULL;
  }
}

/*
 * Writes to the file or console and returns them number of bytes written
 */
void write(struct intr_frame *f){
  int fd = *((int*)(f->esp+4));
  const void *buffer = *((void**)(f->esp+8));
  unsigned size = *((unsigned*)(f->esp+12));
  char out;
  int i = 0;
  if(fd == 1){
    putbuf(buffer, size);
    f->eax = size;
  }
  else if (valid_fd(fd) && file_is_open(fd) && fd != 0){
    file_write(thread_current()->f_container[fd], buffer, size);
    f->eax = size;
  }
  else{
    f->eax = -1;
  }
}

/*
 * Reads from the file or console and returns the nubmer of bytes read
 */
void read(struct intr_frame *f){
  int fd = *((int*)(f->esp+4));
  char *buffer = *((char**)(f->esp+8));
  unsigned size = *((unsigned*)(f->esp+12));
  struct file *arg_f = NULL;

  if (1 < fd && fd < MAX_FD){
    arg_f = thread_current()->f_container[fd];
  }

  if (arg_f != NULL){
    f->eax = file_read(arg_f, buffer, size);
  }
  else if (fd == 0){
    int i;
    for(i=0; i < size; i++){
      buffer[i] = input_getc();
    f->eax = size;
    }
  }
  else{
    f->eax = -1;
  }
}

/*
 * Terminates the current user program
 */
void exit(struct intr_frame *f){
  int status = *((int*)(f->esp+4));
  thread_exit();
}

void exec(struct intr_frame *f){
  char *filename = *((char**)(f->esp+4));
  process_execute(filename)
}
