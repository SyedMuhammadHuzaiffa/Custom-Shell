#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <sys/statvfs.h>
#include <limits.h>
#include <time.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024
#define MAX_NUM_ARGS 64
#define HOST_NAME_MAX 128

// Function to read a line of input
char *read_line()
{
    char *line = NULL;
    size_t bufsize = 0;
    getline(&line, &bufsize, stdin);
    return line;
}

// Function to split input line into arguments
char **parse_line(char *line)
{
    size_t bufsize = MAX_NUM_ARGS;
    size_t position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if (!tokens)
    {
        fprintf(stderr, "Allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, " \t\n");
    while (token != NULL)
    {
        tokens[position] = token;
        position++;

        if (position >= bufsize)
        {
            bufsize += MAX_NUM_ARGS;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens)
            {
                fprintf(stderr, "Allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, " \t\n");
    }
    tokens[position] = NULL;
    return tokens;
}

// custom commands handlers

void custom_echo(char **args)
{
    int i = 1;
    while (args[i] != NULL)
    {
        printf("%s ", args[i]);
        i++;
    }
    printf("\n");
}

void custom_cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "shell: expected argument to \"cd\"\n");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("shell");
        }
    }
}

void custom_ls()
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir(".");
    if (dir == NULL)
    {
        perror("shell");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        printf("%s\n", entry->d_name);
    }

    closedir(dir);
}

void custom_pwd()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("%s\n", cwd);
}

void custom_uname()
{
    struct utsname uname_data;
    if (uname(&uname_data) == -1)
    {
        perror("shell");
        return;
    }

    printf("%s\n", uname_data.sysname);
    printf("%s\n", uname_data.release);
    printf("%s\n", uname_data.version);
    printf("%s\n", uname_data.machine);
}

void custom_mkdir(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "usage: mkdir [-pv] [-m mode] directory_name ...\n");
        return;
    }

    int i = 1;
    while (args[i] != NULL)
    {
        if (mkdir(args[i], 0777) == -1)
        {
            perror("shell");
        }
        i++;
    }
}

void custom_rmdir(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "usage: rmdir directory_name ...\n");
        return;
    }

    int i = 1;
    while (args[i] != NULL)
    {
        if (rmdir(args[i]) == -1)
        {
            perror("shell");
        }
        i++;
    }
}

void custom_clear()
{
    printf("\033[H\033[J"); //\033 is the escape character. [ is the left bracket character. H is the ANSI code for setting the cursor position. So, \033[H means move the cursor to the home position (upper-left corner of the terminal).\033[J is the ANSI code for clearing the screen from the cursor position to the end of the screen.
}

void custom_rm(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "usage: rm file_name ...\n");
        return;
    }

    int i = 1;
    while (args[i] != NULL)
    {
        if (remove(args[i]) == -1)
        {
            perror("shell");
        }
        i++;
    }
}

void custom_touch(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "usage: touch file_name ...\n");
        return;
    }

    int i = 1;
    while (args[i] != NULL)
    {
        if (open(args[i], O_CREAT | O_WRONLY | O_EXCL, 0666) == -1)
        {
            perror("shell");
        }
        i++;
    }
}
void custom_locate(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "usage: locate file_name ...\n");
        return;
    }

    int i = 1;
    while (args[i] != NULL)
    {
        char *file_name = args[i];
        char command[256];
        snprintf(command, sizeof(command), "find . -name %s -type f", file_name);
        system(command);
        i++;
    }
}

void custom_file(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "usage: file file_name ...\n");
        return;
    }

    int i = 1;
    while (args[i] != NULL)
    {
        struct stat file_stat;
        if (stat(args[i], &file_stat) == -1)
        {
            perror("shell");
        }
        else
        {
            const char *file_type = "unknown file type";
            if (S_ISREG(file_stat.st_mode))
            {
                file_type = "regular file";
            }
            else if (S_ISDIR(file_stat.st_mode))
            {
                file_type = "directory";
            }
            else if (S_ISLNK(file_stat.st_mode))
            {
                file_type = "symbolic link";
            }

            printf("%s: %s\n", args[i], file_type);
        }
        i++;
    }
}

void custom_df()
{
    struct statvfs stat;
    if (statvfs(".", &stat) == -1)
    {
        perror("shell");
        return;
    }
    unsigned long long free_size = stat.f_bfree * stat.f_frsize;
    unsigned long long available_size = stat.f_bavail * stat.f_frsize;

    unsigned long long total_size = 0;
    printf("Total size: %llu bytes\n", total_size);
    printf("Free size: %llu bytes\n", free_size);
    printf("Available size: %llu bytes\n", available_size);
}

void custom_ps()
{
    FILE *fp;
    char path[1035];

    // Open the command for reading
    fp = popen("ps -ef", "r");
    if (fp == NULL)
    {
        perror("shell");
        return;
    }

    // Read the output line by line
    while (fgets(path, sizeof(path), fp) != NULL)
    {
        printf("%s", path);
    }

    // Close the file pointer
    pclose(fp);
}

void custom_hostname()
{
    char hostname[HOST_NAME_MAX + 1];
    if (gethostname(hostname, HOST_NAME_MAX + 1) == -1)
    {
        perror("shell");
        return;
    }

    printf("Hostname: %s\n", hostname);
}

void custom_time()
{
    time_t current_time;
    struct tm *time_info;
    char time_string[9]; // HH:MM:SS\0

    time(&current_time);
    time_info = localtime(&current_time);

    strftime(time_string, sizeof(time_string), "%H:%M:%S", time_info);

    printf("Current time: %s\n", time_string);
}

void custom_date()
{
    time_t current_time;
    struct tm *time_info;
    char date_string[11]; // YYYY-MM-DD\0

    time(&current_time);
    time_info = localtime(&current_time);

    strftime(date_string, sizeof(date_string), "%Y-%m-%d", time_info);

    printf("Current date: %s\n", date_string);
}

void custom_kill(char **args)
{
    // Get the process ID from the command line argument
    int pid = atoi(args[1]);

    // Send the SIGTERM signal to the specified process
    if (kill(pid, SIGTERM) == -1)
    {
        perror("mkill");
        return;
    }

    printf("Process with PID %d killed\n", pid);
}

void custom_netstat()
{
    // Open the command for reading
    FILE *fp = popen("netstat -tuln", "r");
    if (fp == NULL)
    {
        perror("shell");
        return;
    }

    // Read the output line by line
    char path[256];
    while (fgets(path, sizeof(path), fp) != NULL)
    {
        printf("%s", path);
    }

    // Close the file pointer
    pclose(fp);
}

void custom_ping(char **args)
{
    // Check if the required arguments are provided
    if (args[1] == NULL)
    {
        printf("Usage: ping <hostname>\n");
        return;
    }

    // Create the command string
    char command[256];
    snprintf(command, sizeof(command), "ping -c 4 %s", args[1]);

    // Open the command for reading
    FILE *fp = popen(command, "r");
    if (fp == NULL)
    {
        perror("shell");
        return;
    }

    // Read the output line by line
    char output[256];
    while (fgets(output, sizeof(output), fp) != NULL)
    {
        printf("%s", output);
    }

    // Close the file pointer
    pclose(fp);
}

void custom_jobs()
{
    // Print the list of background processes
    printf("List of background processes:\n");

    // Open the command for reading
    FILE *fp = popen("ps -ef", "r");
    if (fp == NULL)
    {
        perror("shell");
        return;
    }

    // Read the output line by line
    char process_info[256];
    while (fgets(process_info, sizeof(process_info), fp) != NULL)
    {
        // Check if the process is a background process
        if (strstr(process_info, "shell") != NULL)
        {
            printf("%s", process_info);
        }
    }

    // Close the file pointer
    pclose(fp);
}

void custom_top()
{
    // Open the command for reading
    FILE *fp = popen("top -n 1", "r");
    if (fp == NULL)
    {
        perror("shell");
        return;
    }

    // Read the output line by line
    char process_info[256];
    while (fgets(process_info, sizeof(process_info), fp) != NULL)
    {
        printf("%s", process_info);
    }

    // Close the file pointer
    pclose(fp);
}

void custom_ifconfig()
{
    // Execute the ifconfig command
    system("ifconfig");
}

void custom_who()
{
    // Open the command for reading
    FILE *fp = popen("who", "r");
    if (fp == NULL)
    {
        perror("shell");
        return;
    }

    // Read the output line by line
    char user_info[256];
    while (fgets(user_info, sizeof(user_info), fp) != NULL)
    {
        printf("%s", user_info);
    }

    // Close the file pointer
    pclose(fp);
}

void custom_cal()
{
    // Execute the cal command
    system("cal");
}

void custom_exit()
{
    exit(0);
}

void custom_rename(char **args)
{
    if (args[1] == NULL || args[2] == NULL)
    {
        printf("Usage: rename <old_filename> <new_filename>\n");
        return;
    }

    int result = rename(args[1], args[2]);
    if (result == 0)
    {
        printf("File renamed successfully.\n");
    }
    else
    {
        perror("Error renaming file");
    }
}

void custom_edit(char **args)
{
    if (args[1] == NULL)
    {
        printf("Usage: edit <filename>\n");
        return;
    }

    char *filename = args[1];
    // Open the file in write mode
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("Error opening file");
        return;
    }

    // Prompt the user to enter the content to be written to the file
    printf("Enter the content (press Ctrl+D to finish):\n");
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        // Write the content to the file
        fputs(buffer, file);
    }

    // Close the file
    fclose(file);
    printf("File edited successfully.\n");
}

void custom_shutdown()
{
    system("shutdown -h now");
}

void custom_restart()
{
    system("shutdown -r now");
}

int main()
{
    char *line;
    char **args;
    int status;

    do
    {
        printf("shell> ");
        line = read_line();
        args = parse_line(line);
        status = 1; // Continue running the shell

        // Checks for custom commands only
        if (args[0] != NULL)
        {
            if (strcmp(args[0], "mecho") == 0)
            {
                custom_echo(args);
            }
            else if (strcmp(args[0], "mcd") == 0)
            {
                custom_cd(args);
            }
            else if (strcmp(args[0], "mls") == 0)
            {
                custom_ls();
            }
            else if (strcmp(args[0], "mpwd") == 0)
            {
                custom_pwd();
            }
            else if (strcmp(args[0], "muname") == 0)
            {
                custom_uname();
            }
            else if (strcmp(args[0], "mmkdir") == 0)
            {
                custom_mkdir(args);
            }
            else if (strcmp(args[0], "mclear") == 0)
            {
                custom_clear();
            }
            else if (strcmp(args[0], "mrmdir") == 0)
            {
                custom_rmdir(args);
            }
            else if (strcmp(args[0], "mrm") == 0)
            {
                custom_rm(args);
            }
            else if (strcmp(args[0], "mtouch") == 0)
            {
                custom_touch(args);
            }
            else if (strcmp(args[0], "mlocate") == 0)
            {
                custom_locate(args);
            }
            else if (strcmp(args[0], "mfile") == 0)
            {
                custom_file(args);
            }
            else if (strcmp(args[0], "mdf") == 0)
            {
                custom_df();
            }
            else if (strcmp(args[0], "mps") == 0)
            {
                custom_ps();
            }
            else if (strcmp(args[0], "mhostname") == 0)
            {
                custom_hostname();
            }
            else if (strcmp(args[0], "mtime") == 0)
            {
                custom_time();
            }
            else if (strcmp(args[0], "ndate") == 0)
            {
                custom_date();
            }
            else if (strcmp(args[0], "mexit") == 0)
            {
                custom_exit();
            }
            else if (strcmp(args[0], "mkill") == 0)
            {
                custom_kill(args);
            }
            else if (strcmp(args[0], "mnetstat") == 0)
            {
                custom_netstat();
            }
            else if (strcmp(args[0], "mping") == 0)
            {
                custom_ping(args);
            }
            else if (strcmp(args[0], "datetime") == 0)
            {
                custom_date();
                custom_time();
            }
            else if (strcmp(args[0], "mjobs") == 0)
            {
                custom_jobs();
            }
            else if (strcmp(args[0], "mtop") == 0)
            {
                custom_top();
            }
            else if (strcmp(args[0], "mifconfig") == 0)
            {
                custom_ifconfig();
            }
            else if (strcmp(args[0], "mwho") == 0)
            {
                custom_who();
            }
            else if (strcmp(args[0], "mcal") == 0)
            {
                custom_cal();
            }
            else if (strcmp(args[0], "mrename") == 0)
            {
                custom_rename(args);
            }
            else if (strcmp(args[0], "medit") == 0)
            {
                custom_edit(args);
            }
            else if (strcmp(args[0], "mshutdown") == 0)
            {
                custom_shutdown();
            }
            else if (strcmp(args[0], "mrestart") == 0)
            {
                custom_restart();
            }
            else
            {
                printf("Error not a command\n");
            }
        }

        free(line);
        free(args);
    } while (status);

    return 0;
}