Bitcoin Private Key Finder
==========================

What is this tool?
------------------

This tool helps you to find private keys in WIF format.

The regex used are:
BITCOIN: `(K[wxyz]|L[1-5]|5[KHJ])[1-9A-HJ-NP-Za-km-z]{49,50}`
LITECOIN: `(T[3456789AB]|6[uvw])[1-9A-HJ-NP-Za-km-z]{49,50}`


Compiling
---------

`g++ findprivate.cpp -I. -lcrypto -o findprivate`


Usage
-----
Here are some examples of how to use the tool

### Search for private keys in disk
`cat /dev/sdb | findprivate -p priv.txt`

### Ignore errors reading disk
`dd if=/dev/sdb bs=1M conv=noerror | findprivate -x priv-hex.txt`

### Search for private keys in files and check with brainflayer
```
cat * | findprivate -x priv-hex.txt
brainflayer -v -b ${BRAINFILE}.blf -f ${BRAINFILE}.bin -t hexbinary -i priv-hex.txt
```


Donations
---------
1PATATASCh385nEXwbPBjoJ95FAuQchPYB