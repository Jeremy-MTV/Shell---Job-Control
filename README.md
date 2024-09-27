# Shell with Job Control

This project is a custom Unix-like shell implemented in C, with job control functionality. It provides a minimal yet powerful environment for executing commands and managing processes, supporting both foreground and background jobs.

## Features

- Basic shell functionalities: executing commands, handling built-in commands.
- Job control: manage background and foreground processes with features like stopping, resuming, and terminating jobs.
- Redirection: input and output redirection for commands.
- Script support: allows for running batch scripts.

## Project Structure

- **src/**: Contains the main source code.
  - `builtin.c`: Handles built-in shell commands.
  - `command.c`: Processes and parses command line input.
  - `execute.c`: Handles the execution of commands.
  - `job.c`: Implements job control functionalities.
  - `parser.c`: Parses shell input.
  - `redirections.c`: Manages input/output redirection.
  - `main.c`: Entry point of the shell.
- **head/**: Header files defining functions and structures used across the shell.
  - `jsh.h`: Main header file for the project.
- **test.sh**: Shell script for testing the functionality of the shell.
- **Makefile**: Contains build instructions for compiling the project.

## Requirements

- **C Compiler**: Ensure you have `gcc` or another C compiler installed.
- **Make**: Used for compiling the project through the Makefile.

## Installation

Clone the repository and navigate to the project directory:

```bash
git clone https://github.com/Jeremy-MTV/Shell---Job-Control.git
cd Shell---Job-Control
```

Compile the project using `make`:

```bash
make
```

## Usage

After compiling, run the shell:

```bash
./jsh
```

You can now execute Unix commands within this shell. To manage jobs, you can use the following commands:

- **`jobs`**: List all background jobs.
- **`bg %<job-id>`**: Resume a stopped job in the background.
- **`fg %<job-id>`**: Bring a job to the foreground.
- **`kill %<job-id>`**: Terminate a job.

## Testing

You can test the shell functionality with the included test script:

```bash
./test.sh
```

This will run through a series of predefined tests to ensure the shell operates as expected.

## Documentation

For more detailed information about the architecture and design decisions, refer to the following:

- **[ARCHITECTURE.md](ARCHITECTURE.md)**: Detailed explanation of the shell's architecture.
- **[AUTHORS.md](AUTHORS.md)**: List of project contributors.

## Contributing

If you'd like to contribute to this project, please follow these steps:

1. Fork the repository.
2. Create a new branch (`git checkout -b feature-branch`).
3. Commit your changes (`git commit -m 'Add some feature'`).
4. Push to the branch (`git push origin feature-branch`).
5. Open a pull request.

## License

This project is licensed under the MIT License.

