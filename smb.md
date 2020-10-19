```shell
[global]
workgroup = WORKGROUP                
server string  = Samba Server Version
netbios name = MYSERVER
unix charset = utf8
dos charset = GB2312 
security = user
map to guest = Bad User
guest account = root
passdb backend = tdbsam

[share]
comment = share
path = /usr/src
public = yes
writable = yes
browseable=yes
available=yes
guest ok=yes
```
