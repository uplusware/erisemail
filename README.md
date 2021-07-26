![erisemail](https://raw.githubusercontent.com/uplusware/erisemail/master/doc/erisemail.png)

# eRisemail - Mail/XMPP Server/Federation based on the Linux/MySQL/Hadoop/LDAP
* Support SMTP, POP3, IMAP4, HTTP, XMPP and SSL/TLS
* Support WebMail and WebAdmin based on built-in web server
* Support CRAM-MD5, DIGEST-MD5, APOP, External/TLS-Certification and GSSAPI/Kerberos
* Support Mail Group, Customize User Policy and Mail audit
* Support popular email client( Outlook, Thunderbird ...)
* Support iCalendar and Mozilla Thunderbird Lightning Plugin
* Support LDAP Address Book based on built-in LDAP server.
* Support the mail with huge size attachment( Maximum size is 2G )
* Support Apache SpamAssassin and [How to use Anti-spam Engine in eRisemail](https://github.com/uplusware/erisemail/wiki/How-to-use-Anti-spam-Engine-in-eRisemail)
* Recommend Ubuntu or CentOS as host OS [Installation](https://github.com/uplusware/erisemail/wiki/Installation)
* Project [Wiki](https://github.com/uplusware/erisemail/wiki)

* Install and configure MariaDB and memcached and their development libraries.
  * Ubuntu16-x86_64bit
      * `sudo apt-get install mariadb-server libmariadb-client-lgpl-dev libmariadbclient-dev memcached libmemcached-dev`
  * Centos7-x86_64bit
      * `sudo yum install mariadb-server.x86_64 mariadb-devel.x86_64 libmemcached-devel.x86_64`

# Install by binary
### Download install package from http://uplusware.org/.  
Run `tar -xzf erisemail_***.gz`  
Run `cd erisemail_filename`  
Edit the file of erisemail-utf8.conf. set domain, database and others.  

* install erisemail:   
`sudo ./install.sh` 
`sudo eriseutil --install`  
Start or Stop the service
* Run `/etc/init.d/erisemail start`  
* Run `/etc/init.d/erisemail stop`   
   
# Install by source code
* `sudo apt-get install gcc g++ make openjdk-8-jdk unzip`
* Enter the directory the source code of erisemail
* Download and build libtasn1
   * Download libasn1 from https://www.gnu.org/software/libtasn1/
   * Extract and rename or `ln -s` it to `libtasn1`,the default path is '${ERISEMAIL_SOURCE}/libtasn1'.
   * `cd libtasn1`
   * `./configure --enable-shared=no`
   * `make`
   * `sudo make install`
* Download and build OpenSSL
   * Download openssl from https://www.openssl.org/source/ into the erisemail source code tree.
       * eg.: `wget https://www.openssl.org/source/openssl-1.0.2j.tar.gz`
   * Extract and rename or `ln -s` it to `openssl`,the default path is '${ERISEMAIL_SOURCE}/openssl'.
   * `cd openssl`
   * `./config no-shared`
   * `make`
   * `make test`
* Download and install Hadoop(If want to support HDFS feature)
   * Download Hadoop http://hadoop.apache.org/releases.html if want HDFS based, the default path is '{ERISEMAIL_SOURCE}/Hadoop'.
   * `cd src/`  
   * `vim Makefile`  
   edit the dir of OPENSSL_DIR, HADOOP_DIR, LIBJVM_DIR and mariadb in the Makefile to your own set.  
   eg. LIBJVM_DIR= /usr/lib/jvm/java-8-openjdk-amd64/jre/lib/amd64/server/  
       -I/usr/include/mariadb/  
* Build erisemail
   * `make`
   * `make LDAP` if want to support LDAP
   * `make HDFS=1` if want HDFS based.
   * `make DIST=1` if want clusting
* `cd ../script`  
* edit the file of erisemail-utf8.conf. set domain, database.  
`vim erisemail-utf8.conf`    
`cd ../`  
* Run `bash build_release_install.sh` in installation package  
* Run `/usr/bin/eriseutil --install`  
start or stop erisemail:   
* Run`/etc/init.d/erisemail start`  
* Run`/etc/init.d/erisemail stop`  
 * Please refer [Online Manual](https://github.com/uplusware/erisemail/wiki/Manual) in the installation package  

* edit hostname to your mail domain.  
`vim /etc/hostname` or `#hostname mail_domain` 
can resolve the error 'Helo command rejected: need fully-qualified hostname';
