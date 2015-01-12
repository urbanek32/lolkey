# lolkey - simple linux keylogger
Simple keylogger made as Linux Kernel Module. Tested with kernel 3.20 on Debian 7(sid)

It's a part of academic project, made in educational purposes.

## Require
 * Linux source and headers
 * ```kbuild```

## Usage
* Download entire repository
* In main directory type ```make```
* ```chmod +x ./load.sh``` then just type ```./load.sh```
* or use ```insmod lolkey.ko```

* To remove module you can use ```./unload.sh``` or type ```rmmod lolkey```

Log file is default stored in /root/log.txt
