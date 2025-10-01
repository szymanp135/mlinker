#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define ERR(s) { fprintf(stderr, "%s:%d ", __FILE__, __LINE__); \
	perror(s); exit(EXIT_FAILURE); }

#define OPTSTRING ":ho:sv"
#define DEFAULT_OUTPUTFILE_NAME "a.out"
#define MLINK_LABEL "mlinker_label_"

#define LINE_LENGTH 1024
#define TOKEN_COUNT 3
#define DELIMITER ';'

/* Print credentials info */
void credentials(void) {
	printf("mlinker by Paweł Szymański 1.10.2025\nVersion v0.1\n\n");
}

/* Print program usage info */
void usage(char* s) {
	printf("Usage: %s -hosv <script>\n\t-h - print help page\n\t-o <f"
	"ilename> - output file name (default: '%s')\n\t-s - output to st"
	"dout instead of file\n\t-v - print version\n\tscript - linker sc"
	"ript\n", s, DEFAULT_OUTPUTFILE_NAME);
}

void help(void) {
	printf("\nLinker script:\nLinker script is a text file that tells"
	" linker what files and how to link. It is\ncompounded of records"
	" of these files. Such records are henceforth called\nsegments. E"
	"ach segment contains three pieces of information:\n1. Linked fil"
	"e name;\n2. Segment starting position in memory;\n3. Segment byt"
	"e size in memory.\nHere memory should be understood as target me"
	"mory where linked program would be\nsaved as binary data after c"
	"ompilation. Each segment in linker script occupies\nexactly one "
	"line. Structure of information given in segment is as follows: f"
	"ile\nname, start position and size, all separated by semicolons."
	" This data shall be\nprovided in said order. Additionally file n"
	"ame shall be surrounded by quote\nsigns of same type.\nSpecial s"
	"ign '*' is provided for both start and size information in segem"
	"nt and may be used instead of real values with following meaning"
	":\n- In case of start it indicates that segment starting positio"
	"n shall be set\n  right after the end of previous one. Should it"
	" be used with first segment it\n  shall be set to 0.\n- In case "
	"of size it indicates that size of segment is of real segemnt siz"
	"e.\n\nExample linker script with explanation:\n\n'init.s;*;$800'"
	"\n\"co'd;e\";*;*\n\nAbove script links two files: \"init.s\" and"
	" \"co'd;e\". Thanks to mandatory quotes\nfile \"co'd;e\" can be "
	"used without changing it's name (characters as \"'\" and ';'\nar"
	"e allowed). File \"init.s\" is put at the start of linked file s"
	"ince it is first\non list. Starting position '*' is used which t"
	"ells it shall be set to 0. Size\nvalue '$800' is used which mean"
	"s size is of 2048 bytes. '$' sign of course\nindicates that valu"
	"e is in hexadecimal. Next linked file is \"co'd;e\". Is starts\n"
	"at whatever position ends previous file. In this case it would b"
	"e at position\n(address) $800 since \"init.s\" segment has such "
	"length and starts at address $0.\nSize of \"co'd;e\" is whatever"
	" it's real size is.\n");
}

/*
 * Reads single character from file.
 *
 * Returns 0 if succeded and non-zero otherwise.
 *
 * Arguments:
 * fd		: File descriptor
 * c		: Pointer for read character
 */
int read_char(int fd, int* c) {
	
	int res;
	int retry = 10;
	
	/* Retry few times to make sure it really is an end of file */
	/* (read returns 0 if EOF) */
	for(; retry > 0; --retry) {
		/* Try to read */
		res = read(fd, (char*)c, 1);
		/* If read correctly then return */
		if(res == 1)
			return 0;
		/* If error then error */
		if(res < 0)
			return -1;
	}
	/* If could not read character then it is EOF */
	*c = EOF;
	return 0;
}

/*
 * Reads line from file.
 *
 * Returns 0 if read successfully; 1 if reached end of file and there is
 * nothing to read; otherwise error.
 *
 * Arguments:
 * fd		: File descriptor
 * line		: Pointer to array to where write read line
 */
int read_line(int fd, char* line) {
	
	int i = 0;
	int c;

	/* Loop reading character by character */
	do {
		if(read_char(fd, &c))
			return -1;
		line[i++] = c;
	  /* Read characters till end of line or EOF */
	} while(c != '\n' && c != EOF);
	/* Last read character is '\n' or EOF so change them to */
	/* terminating null byte */
	line[i - 1] = 0;

	/* If i == 1 and c == EOF then file has been completely read */
	if(i == 1 && c == EOF)
		return 1;
	else
		return 0;
}

/*
 * Splits given string into tokens delimited with 'delim' character.
 * If found quotes ignores it's content (including delimiter character).
 *
 * Returns number of found tokens.
 *
 * Arguments:
 * tokens	: Array of pointers to tokens in 'line' string
 * max_token_count	: Maximum token count to be searched for
 * delim	: Delimiting character
 * line		: Line to be tokenized
 */
int tokenize(char** tokens, int max_token_count, char delim, char* line) {
	
	int i, j;
	int token_count;
	int len;
	int in_quotes = 0;
	char quote_type;

	/* clear tokens */
	for(i = 0; i < max_token_count; ++i)
		memset(tokens[i], 0, LINE_LENGTH);

	/* Look through line char by char */
	for(i = 0, j = 0, len = strlen(line), token_count = 0; i < len &&
		token_count < max_token_count; ++i) {
		/* If not in quotes */
		if(!in_quotes) {
			/* If found quote then change state to in quotes */
			if(line[i] == '\'' || line[i] == '"') {
				quote_type = line[i];
				in_quotes = 1;
				continue;
			}
			/* If found delimiter */
			if(line[i] == delim) {
				/* Finish token by writing terminating */
				/* null byte and prepare for next token */
				tokens[token_count++][j] = 0;
				j = 0;
				continue;
			}
		}
		/* If in quotes then look for ending one */
		else if(line[i] == quote_type) {
			in_quotes = 0;
			continue;
		}
		/* Save characters to tokens */
		tokens[token_count][j++] = line[i];

	}

	/* Write terminating null byte at the end of last token */
	tokens[token_count][j] = 0;

	return token_count + 1;
}

/*
 * Convert character to it's digit value.
 * If character's value is greater or equal than base value
 * error is returned.
 * 
 * Returns 0 if succeeded and non-zero otherwise.
 *
 * Arguments:
 * c		: Character to be converted
 * d		: Value of converted character
 * base		: Base of used system
 */
int char2digit(char c, int* d, int base) {

	*d = 0x7fff;

	if(c >= '0' && c <= '9')
		*d = c - '0';
	else if(c >= 'A' && c <= 'F')
		*d = c - 'A' + 10;
	else if(c >= 'a' && c <= 'f')
		*d = c - 'a' + 10;
	
	if(*d >= base)
		return -1;
	return 0;
}

/* 
 * Converts given token to int value.
 *
 * Returns converted value if succeeded (i.e. positive value); -1 means
 * '*' character; otherwise error.
 *
 * Arguments:
 * token	: Token to be converted
 */
int token2int(char* token) {
	
	int i, d;
	int base;
	int num;

	/* Check whether token is not empty */
	if(strlen(token) < 1)
		return -2;
	
	/* Determine base */
	switch(token[0]) {
		case '$': base = 16; i = 1; break;
		case '0': base =  8; i = 1; break;
		case '*': return -1; break;
		default: {
			if(token[0] > 0 && token[0] <= '9')
				base = 10;
			else
				return -3;
			i = 0;
		} break;
	}

	/* Convert token to int */
	for(num = 0; token[i]; i++) {
		/* Shift value by base factor */
		num *= base;
		/* Read next digit */
		if(char2digit(token[i], &d, base))
			return -1;
		/* Add digit */
		num += d;
	}

	return num;
}

/*
 * Copies data from one file to another one.
 *
 * Returns 0 if succeeded and non-zero otherwise.
 *
 * Arguemnts:
 * fd_src	: Source file descriptor
 * fd_dst	: Destination file descriptor
 */
int copy_file_to_file(int src_fd, int dst_fd) {
	
	int res;
	int wr, tot_wr;
	char buffer[256];

	res = 1;
	while(res) {
		res = read(src_fd, buffer, 256);
		if(res < 0)
			ERR("segment read");
		tot_wr = 0;
		while(tot_wr != res) {
			wr = write(dst_fd, buffer + tot_wr, res - tot_wr);
			if(wr < 0)
				ERR("segment write");
			tot_wr += wr;
		}
	}

	return 0;
}

#define BUFFER_SIZE 4096
int write_segment_code(char* segment_name, int outfile_fd) {

	int res;
	int segment_fd;

	char* segment_code = 0;
	int segment_code_alloc = 0;
	int segment_code_len = 0;

	int written = 0;

	/* Open segment file */
	segment_fd = open(segment_name, O_RDONLY);
	if(segment_fd < 0)
		return -3;
	
	/* Read whole segment file */
	res = 1;
	while(res) {
		/* Allocate more space in case it runs out */
		if(segment_code_len >= segment_code_alloc - BUFFER_SIZE) {
			segment_code_alloc += BUFFER_SIZE;
			segment_code = realloc(segment_code,
				segment_code_alloc);
			if(!segment_code)
				return -1;
		}

		/* Read segment code */
		res = read(segment_fd, segment_code + segment_code_len,
			BUFFER_SIZE);
		if(res < 0)
			return -2;
		segment_code_len += res;
	}
	/* Write new line character at the end of segment code */
	segment_code[segment_code_len] = '\n';

	/* Close segment file */
	close(segment_fd);

	/* Write segment code to linked file */
	written = 0;
	while(written < segment_code_len) {
		res = write(outfile_fd, segment_code + written,
			segment_code_len - written);
		if(res < 0)
			return -3;
		written += res;
	}

	/* Free allocated memory for segment code */
	if(segment_code)
		free(segment_code);

	return 0;
}

int main(int argc, char** argv) {
	
	int i;
	int res;
	int opt;
	int read_opts;
	int verbose = 1;
	char* script_name;
	char* outfile_name = DEFAULT_OUTPUTFILE_NAME;
	int script_fd;
	int outfile_fd;
	char line[LINE_LENGTH];
	char** tokens;
	int start, size;
	int segment_num = 0;

	/* Check if script file has been provided */
	if(argc < 2) {
		usage(argv[0]);
		exit(EXIT_SUCCESS);
	}
	
	/* Read options */
	read_opts = 1;
	while(read_opts) {
		opt = getopt(argc, argv, OPTSTRING);
		switch(opt) {
			case 'h':
				usage(argv[0]);
				help();
				exit(EXIT_SUCCESS);
			case 'o':
				outfile_name = optarg;
				break;
			case 's':
				outfile_fd = 1;
				outfile_name = 0;
				verbose = 0;
				break;
			case 'v':
				credentials();
				exit(EXIT_SUCCESS);
			case '?':
				fprintf(stderr, "No such option as -%c\n",
					argv[optind - 1][1]);
				usage(argv[0]);
				exit(EXIT_FAILURE);
			case ':':
				fprintf(stderr, "Missing option argument "
					"for -%c\n", argv[optind - 1][1]);
				usage(argv[0]);
				exit(EXIT_FAILURE);
			case -1:
				read_opts = 0;
				break;
			default:
				ERR("getopt");
		}
	}
	
	/* Since getopt pushes non-option values to the end and only */
	/* one value is non option and it is necessary for program to */
	/* work, last value has to be script name itself. */
	/* Check if there is script name */
	if(optind >= argc) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	/* Get script name */
	script_name = argv[optind];

	/* Print script and output file name */
	if(verbose)
		printf("Linker script file: '%s'\nOutput file: '%s'\n\n",
			script_name,
			!outfile_name ? "stdout" : outfile_name);

	/* Open linker script and output files */
	script_fd = open(script_name, O_RDONLY);
	if(outfile_name)
		outfile_fd = open(outfile_name, O_WRONLY | O_CREAT |
			O_TRUNC, 0644);
	if(script_fd < 0 || outfile_fd < 0)
		ERR("open");
	
	/* Allocate space for tokens */
	tokens = malloc((TOKEN_COUNT + 1) * sizeof(char*));
	if(!tokens)
		ERR("token malloc");
	for(i = 0; i < TOKEN_COUNT; ++i) {
		tokens[i] = malloc(LINE_LENGTH * sizeof(char));
		if(!tokens[i])
			ERR("token malloc");
	}

	/* Read script fiel line by line and link */
	while((res = read_line(script_fd, line)) == 0) {
		/* Tokenize line */
		res = tokenize(tokens, TOKEN_COUNT, DELIMITER, line);
		if(res != TOKEN_COUNT) {
			fprintf(stderr, "ERROR: Wrong linker script "
				"format.\n");
			exit(EXIT_FAILURE);
		}

		/* Check if segment file has the */
		/* same name as output file */
		if(!strcmp(tokens[0], outfile_name)) {
			fprintf(stderr, "ERROR: Segmant file name same as"
				"output file name.\nYour file %s may be"
				"damaged.\n", outfile_name);
			exit(EXIT_FAILURE);
		}

		/* Convert start and size token to meaningful integers */
		start = token2int(tokens[1]);
		size  = token2int(tokens[2]);
		if(start < -1 || size < -1) {
			fprintf(stderr, "ERROR: Bad start/size value "
				"format:\n\t%s\n", line);
			exit(EXIT_FAILURE);
		}

		/* If verbose then print some info */
		if(verbose) {
			printf("Segment: '%s'\n", tokens[0]);
		}

		/* Write comment with info about linkage */
		dprintf(outfile_fd, "; mlinker - linked file '%s'\n", tokens[0]);
		/* If start position is known then write it */
		if(start >= 0)
			dprintf(outfile_fd, "\tORG $%X\n", start);
		/* Write label so that further can have desired size */
		dprintf(outfile_fd, "%s%d:\n\n", MLINK_LABEL, segment_num);

		/* Write segment code to linked code */
		if(write_segment_code(tokens[0], outfile_fd))
			ERR("segment copy");

		/* If start is known and size is provided write new origin */
		if(start >= 0 && size >= 0) {
			dprintf(outfile_fd, "\n\tORG $%X\n", start +
				size);
		}

		/* Few new liens to separate different */
		/* segments from each other*/
		dprintf(outfile_fd, "\n\n\n");

		segment_num++;
	}
	if(res < 0)
		ERR("read line");

	if(verbose) {
		printf("\nSuccessfully linked segments to %s file.\n",
			outfile_name);
	}

	for(i = 0; i < TOKEN_COUNT; ++i)
		free(tokens[i]);
	free(tokens);
	close(outfile_fd);
	close(script_fd);
	return EXIT_SUCCESS;
}
