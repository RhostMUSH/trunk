#!/bin/bash
ps u -p $(cat ./netrhost.pid)
echo ""
echo $(uptime 2>/dev/null)
iostat -y > /dev/null 2>&1
if [ $? -eq 0 ]
then
   iostat -xd 1 1|grep  "^Device" && iostat -xd -y 1 3|egrep -v "(^Device|^$)"|awk '{if(NR>1)print}'|sort
else
   iostat -xd 1 1|grep  "^Device" && iostat -xd 1 3|egrep -v "(^Device|^$)"|awk '{if(NR>1)print}'|sort
fi
