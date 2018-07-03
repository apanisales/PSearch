# PSearch (Parallel Search Tool)

A Unix utility that recursively searches for matching file names

- Detects the number of online cores and uses this number as the default upper bound for threads launched by the program
    - Launch one thread per search term
    - If a user were to specify more search terms than there are threads available, then the program uses a semaphore to wait until a thread finishes before starting another

Usage:
```
$ ./psearch [-eh] [-d directory] [-t threads] search_term1 search_term2 ... search_termN
```
- **-d directory**: specifies start directory (default: Current working directory)
- **-e**: print exact name matches only
- **-h**: show usage information
- **-t threads**: set maximum threads (default: Number of CPUs)
