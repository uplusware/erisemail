./release.sh 1.6.08 utf8 ubuntu
sudo /etc/init.d/erisemail stop
sudo ./ubuntu-erisemail-bin-en-utf8-x86_64-linux/install.sh
sudo /etc/init.d/erisemail start