# strcmp or hash

I wrote this simple benchmark application to compare traditional glibc's `strcmp` function against some kind of string hash compare methods (we're using `uthash` here but it can be extended as well).

You can test with your custom, newline separated dictionary files too.

## build

```
$ git clone https://github.com/demirten/strcmp-or-hash.git

$ cd strcmp-or-hash

$ git submodule update --init --recursive

$ make
```

## run

You need a sample dictionary file. There are lots of good examples in https://github.com/berzerk0/Probable-Wordlists.

Here an example output of my machine: Debian-10, Intel(R) Core(TM) i7-8700 CPU @ 3.20GHz

```
$ wget https://raw.githubusercontent.com/danielmiessler/SecLists/master/Passwords/xato-net-10-million-passwords-100000.txt

$ ./strcmp-or-hash -f xato-net-10-million-passwords-100000.txt -t strcmp
Total Time: 25.164587133 seconds

$ ./strcmp-or-hash -f xato-net-10-million-passwords-100000.txt -t uthash
Total Time: 0.006003785 seconds
```

