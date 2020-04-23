Bitcoin Private Key Finder
==========================

What is this tool?
------------------

This tool helps you to find possible private keys in disk or files

The tool search for `\x04\x20` bytes and extract the next 32 bytes.


Compiling
---------

`gcc findkey.c -o findkey`


Usage
-----
Here are some examples of how to use the tool

### Search for private keys in disk
`cat /dev/sdb | findkey -x priv-hex.txt`

### Ignore errors reading disk
`dd if=/dev/sdb bs=1M conv=noerror | findkey -x priv-hex.txt`

### Search for private keys in files and check with brainflayer
```
cat wallet.dat | findkey -x priv-hex.txt
brainflayer -v -b ${BRAINFILE}.blf -f ${BRAINFILE}.bin -t hexbinary -i priv-hex.txt
```


Donations
---------
1PATATASCh385nEXwbPBjoJ95FAuQchPYB