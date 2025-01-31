# bibutils

Updated by Adam Erickson

## Description

The `bibutils` program set convert between various bibliography formats using a common MODS-format XML intermediate. For example, one can convert RIS-format files to BibTeX by doing two transformations: RIS -> MODS -> Bibtex. By using a common intermediate for N formats, only 2N programs are required and not N²-N. These programs operate on the command line and are styled after standard UNIX-like filters.

I primarily use these tools at the command line, but they are suitable for scripting and have been incorporated into a number of different bibliographic projects.

The Library of Congress's Metadata Object Description Schema (MODS) XML format is described here. I've written a short introduction to using MODS that might be useful.

## Programs

| Program         | Description                                                                    |
| --------------- | ------------------------------------------------------------------------------ |
| `bib2xml`       |    convert BibTeX to MODS XML intermediate                                     |
| `biblatex2xml`  |    convert BibLaTeX to MODS XML intermediate                                   |
| `bibdiff`       |    compare two bibliographies after reading into the bibutils internal format  |
| `copac2xml`     |    convert COPAC format references to MODS XML intermediate                    |
| `end2xml`       |    convert EndNote (Refer format) to MODS XML intermediate                     |
| `endx2xml`      |    convert EndNote XML to MODS XML intermediate                                |
| `isi2xml`       |    convert ISI web of science to MODS XML intermediate                         |
| `med2xml`       |    convert Pubmed XML references to MODS XML intermediate                      |
| `modsclean` 	  |    a MODS to MODS converter for testing puposes mostly                         |
| `nbib2xml`      |    convert Pubmed/NLM nbib format to MODS XML intermedidate                    |
| `ris2xml`       |    convert RIS format to MODS XML intermediate                                 |
| `xml2ads`       |    convert MODS XML intermediate into SAO/NASA ADS reference format            |
| `xml2bib`       |    convert MODS XML intermediate into BibTeX                                   |
| `xml2biblatex`  |    convert MODS XML intermediate into BibLaTeX                                 |
| `xml2end`       |    convert MODS XML intermediate into format for EndNote                       |
| `xml2isi`       |    convert MODS XML intermediate to ISI format                                 |
| `xml2nbib`      |    convert MODS XML intermediate to Pubmed/NLM nbib format                     |
| `xml2ris`       |    convert MODS XML intermediate into RIS format                               |
| `xml2wordbib`   |    convert MODS XML intermediate into Word 2007 bibliography format            |

## Development History

Version 8. Forked from the original project at version 7.2.
Version 7. Library changes forced change of major version.
Version 6. Library changes forced change of major version.
Version 5. User visible changes change default input and output character set to UTF-8 Unicode and ability to write MODS XML in multiple character sets.
Version 4. User visible changes are alteration of library API.
Version 3.
Version 2.
Version 1.

## Frequently Asked Questions

The programs don't run for me. What am I doing wrong? If you send me this question, I would immediately have to ask for more information. So do us both a favor and provide more information. The follow items address specific problems:

"command not found" The message "command not found" on Linux/UNIX/MacOSX systems indicates that the commands cannot be found. This could mean that the programs are not flagged as being executable (run "chmod ugo+x xml2bib" for the appropriate binaries) or the executables are not in your current path (and have to be specified directly like ./xml2bib). A quick web search on chmod or path variables should provide many detailed resources.

I'm running MacOSX and I just get a terminal when I double-click on the programs. Simply put, this is not the way to run the programs. You want to run the terminal first and then issue the commands at the command line. It should be under Applications->Utilities->Terminal on most MacOSX systems I've seen. If you just double-click the program, the terminal corresponds to the input to the tool. Not so useful. Some links to get you started running the terminal in a standard UNIX-like fashion are at TerminalBasics.pdf, macdevcenter.com, and ee.surrey.ac.uk.

This stuff is great, how can I help? OK, I actually don't get this question so often, though I have gotten very useful help through people who have willingly sent useful bug reports and sample problematic data to allow me to test these programs. I willingly accept bug reports, patches, new filters, suggestions on program improvements or better documentation and the like. All I can say is that users (or programmers) who contact me with these sorts of things are far more likely to get their itches scratched.

I am interested in bug reports and problems in conversions. Feel free to e-mail me if you run into these issues. The absolute best bug reports provide error messages from the operating systems and/or input and outputs that detail the problems. Please remember that I'm not looking over your shoulder and I cannot read your mind to figure out what you are doing--"It doesn't work." isn't a bug report I can help you with.
License

All versions of bibutils are relased under the GNU Public License (GPL) version 2. In a nutshell, feel free to download, run, and modify these programs as required. If you re-release these, you need to release the modified version of the source. (And I'd appreciate patches as well...if you care enough to make the change, then I'd like to see what you're adding or fixing.)

## Compilation

### 1. Configure the Makefile by running the `configure` script

The configure script attempts to auto-identify your operating system
and does a reasonable job for a number of platforms (including x86 Linux,
versions of MacOSX, some BSDs, Sun Solaris, and SGI IRIX).  It's not a 
full-fledged configure script via the autoconf system, but is more than 
sufficient for Bibutils.

Unlike a lot of programs, Bibutils is written in very vanilla ANSI C
with no external dependencies (other than the core C libraries themselves),
so the biggest difference between platforms is generally how they
handle library generation.  If your platform is not recognized, please
e-mail me the output of 'uname -a' and I'll work on adding it.

To configure the makefile, simply run:

```shell
configure
```

or alternatively

```shell
sh -f configure
```

The output should look something like:

```shell
Bibutils Configuration
----------------------

Operating system:               Linux_x86_64
Library and binary type:        static
Binary installation directory:  /usr/local/bin
Library installation directory: /usr/local/lib

 - If auto-identification of operating system failed, e-mail cdputnam@ucsd.edu
   with the output of the command: uname -a

 - Use --static or --dynamic to specify library and binary type;
   the --static option is the default

 - Set binary installation directory with:  --install-dir DIR

 - Set library installation directory with: --install-lib DIR


To compile,                  type: make
To install,                  type: make install
To make tgz package,         type: make package
To make deb package,         type: make deb

To clean up temporary files, type: make clean
To clean up all files,       type: make realclean
```

By default, the configure script generates Makefiles to generate statically
linked binaries.  These binaries are the largest, but require no management of
dynamic libraries, which can be subtle for users not used to installing
them and ensuring that the operating system knows where they are.
Dynamically linked binaries take up substantially less disk space, but require
real machine and distribution specific knowledge for handling the dynamic
library installation and usage.  All of the distributed binaries are statically
linked for obvious reasons.

### 2. Make the package using `make`

```shell
make
```

### 3. Install the package using `make`

```shell
make install
```

Note that `make install` won't install the libraries with statically-linked binaries but will (naturally) with dynamically-linked binaries.

