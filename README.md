kde-workspace
=============

Add MS Windows Task Switcher behaviour to kde-workspace 

This kde-workspace is cloned from git://anongit.kde.org/kde-workspace,
to add alternative behaviour for task switching as wished in:

https://forum.kde.org/viewtopic.php?f=111&t=99625
and
https://bugs.kde.org/show_bug.cgi?id=297445


If u want to treat minimized windows like in the ms windows os, and have a 64-bit kubuntu 14, u can download the kwin compiled version and replace your local:

`wget "https://github.com/katakumpo/kde-workspace/blob/KDE/4.11/build/kwin?raw=true" -O /tmp/kwin`

`cd /usr/bin`

`sudo mv kwin kwin_original`

`mv /tmp/kwin .`

`chmod a+x kwin`


else you need to compile it yourself https://community.kde.org/KWin/Building
