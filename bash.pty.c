

#include <paths.h> // has _PATH_BSHELL defined
#include <errno.h>
#include <fcntl.h> 
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>

// Unix98 Method of Creating Pseudo Terminals
//

// API's 
//  + posix_openpt
//  + grantpt
//  + unlockpt
//  + ptsname

void printTTYS(char* message) {
	printf("§ %s\n", message);
	printf("=============================\n");
    system("ls -l /dev/ttys00*");
}

void master_provider(int master_pty_fds, int slave_pty_fds)
{
	// close the salve PTY;
	// because the master does not need the slave file descriptor
	// TODO: let's leave this and see what happens
	close(slave_pty_fds);


	char buffer[150];
	char msg[] = "• [MASTER] Input\n";
	write(STDOUT_FILENO, msg, sizeof(msg));
	int rc;


	// what we want to do here by making it generic.
	// we listen to both the master_fd and stdin
	// when data is available from the slave, it will be read from master_fd
	// when user types in commands, those will be available from stdin
	//
	
	fd_set fd_ins; // fild descriptor inputs

	close(slave_pty_fds);

	while (1) 
	{
		FD_ZERO(&fd_ins); // reset the file descriptor inputs
		FD_SET(STDIN_FILENO, &fd_ins); // add STDIN to the set
		FD_SET(master_pty_fds, &fd_ins); // add Master PTY to the set

		rc = select(master_pty_fds + 1,
			        &fd_ins, 0, 0, 0);

		switch(rc)
		{
			case -1: 
				fprintf(stderr, "Error code:[%d] on select().", errno);
				exit(1);
			default:
				// user entered data in the STDIN
				if (FD_ISSET(STDIN_FILENO, &fd_ins)) 
				{
					rc = read(STDIN_FILENO, buffer, sizeof(buffer));
					if (rc > 0) 
					{
						buffer[rc - 1] = '\0';
						printf("[MASTER] User typed in '%s'.\n", buffer);
						buffer[rc - 1] = '\n';
						write(master_pty_fds, buffer, rc);
					}
				} 

				// SLAVE sends some data and we need to read it
				if (FD_ISSET(master_pty_fds, &fd_ins))
				{
					rc = read(master_pty_fds, buffer, sizeof(buffer));
					if (rc > 0)
					{
						buffer[rc - 1] = '\0';
						printf("[MASTER] Received from slave '%s'.\n", buffer);
						buffer[rc - 1] = '\0';
						write(STDOUT_FILENO, buffer, rc);
					}
				}
		}
	}
}

void slave_provider(int master_pty_fds, int slave_pty_fds)
{
	struct termios original_settings;
	struct termios new_settings;

	char buffer[150];

	// close the master PTY
	close(master_pty_fds);

	// save before we override all the shit
	int rc = tcgetattr(slave_pty_fds, &original_settings);
	new_settings = original_settings; // copy by value
	printf("• [SLAVE] New Settings: new[%d] original[%d]\n", 
		(int)&new_settings, (int)&original_settings);
	cfmakeraw(&new_settings);
	tcsetattr(slave_pty_fds, TCSANOW, &new_settings);

	// close the current terminal STD's
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	// assign PTY as the STD's
	dup(slave_pty_fds);
	dup(slave_pty_fds);
	dup(slave_pty_fds);

	close(slave_pty_fds);

	setsid();
	ioctl(STDIN_FILENO, TIOCSCTTY, 0);

	// let's get the shell
	const char* shell = getenv("SHELL");
	if (!shell)
		shell = _PATH_BSHELL;

	printf("[SLAVE] shell path is '%s'.\n", shell);
	execl(shell, shell, "-i", 0);
	/**
	while (1) 
	{
		rc = read(STDIN_FILENO, buffer, sizeof(buffer));
		if (rc > 0) 
		{
			// the buffer[rc] contains the NULL
			// buffer[rc - 1] contains the newline
			// trim it by replace newline with a NULL
			buffer[rc - 1] = '\0';
			printf("[SLAVE] Received [%d] bytes from MASTER: %s\n", rc, buffer);
		}
		else
		{
			printf("[SLAVE] Nothing read. Breaking.");
			//break;
		}
	}
	**/
}

int main(void)
{
	int fdm; // file descriptor main
	int fds; // file descriptor slave
	int rc;  // return code

    printTTYS("At the Start");

	// opens first available master PTY device 
	// and retutnrs a descriptor
	fdm = posix_openpt(O_RDWR);
	printf("fdm:[%d]\n", fdm);
	if (fdm < 0) 
	{
		fprintf(stderr, "Error code:[%d] posix_openpt.", errno);
		return 1;
	}

	// change the access rights on the salve side
	rc = grantpt(fdm);
	if (rc < 0) 
	{
		fprintf(stderr, "Error code:[%d] grantpt.", errno);
		return 1;
	}

	// unlock the slave side
	rc = unlockpt(fdm);
	if (rc < 0) 
	{
		fprintf(stderr," Error code:[%d] unlockpt.", errno);
		return 1;
	}

	printTTYS("At the End");

	printf("\n\n• Slave side name '%s'.\n", ptsname(fdm));


	// open the slave 
	fds = open(ptsname(fdm), O_RDWR);


	// let's fork it here and see the valudes of FDM and FDS.
	int child_process_id = fork();
	char* process_name = "ERROR";
	if (child_process_id < 0) 
	{
		// error
		fprintf(stderr, "Error code:[%d] fork.", errno);
		return 1;
	} 
	else if (child_process_id > 0)
	{
		process_name = "MASTER";
	} 
	else 
	{
		process_name = "CHILD";
	}
	printf("• %s:[%d] FDM:[%d] FDS:[%d]\n", process_name, child_process_id, fdm, fds);

	if (child_process_id > 0) master_provider(fdm, fds);
	else if (child_process_id == 0) slave_provider(fdm, fds);
	/**
	  the idea here is that.
	  the main process 
	  • writes to the MASTER
	  • reads from the MASTER
	  
	  the slave process
	  keep writing to the MASTER
	  and re
	*/

	return 0;
}