Bitcoin Private Key Finder
==========================

What is this tool?
------------------

This tool helps you to find private keys in WIF format.

The regex used is `(K[wxyz]|L[1-5]|5[KHJ])[1-9A-HJ-NP-Za-km-z]{49,50}`

Compiling
---------

`g++ findprivate.cpp -I. -lcrypto -o findprivate`

Usage
-----
Here are some examples of how to use the tool

### Search for private keys in disk
`cat /dev/sdb | findprivate -p priv.txt`

### Search for private keys in files and check with brainflayer
```
cat * | findprivate -x priv-hex.txt
brainflayer -v -b ${BRAINFILE}.blf -f ${BRAINFILE}.bin -t hexbinary -i priv-hex.txt
```



Donations
---------
1PATATASCh385nEXwbPBjoJ95FAuQchPYB