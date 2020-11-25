#!/bin/bash
pmap -V > /dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "Memory tool not installed."
   exit 1
fi

printf "" > /dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "String formatter not installed."
   exit 1
fi

gl_mem="$(pmap -x $(cat netrhost.pid))"
gl_anon="$(echo "${gl_mem}"|grep "\[ anon \]"|awk '{print $2}')"
gl_so="$(echo "${gl_mem}"|grep "\.so"|awk '{print $2}')"
gl_tmp="$(echo "${gl_mem}"|grep -v "\.so")"
gl_other="$(echo "${gl_tmp}"|grep -v "\[ anon \]"|awk '{print $2}')"
gl_i_anon=0
gl_i_so=0
gl_i_other=0
for i in ${gl_anon}
do
   ((gl_i_anon=${gl_i_anon}+i))
done

for i in ${gl_so}
do
   ((gl_i_so=${gl_i_so}+i))
done

for i in ${gl_other}
do
   ((gl_i_other=${gl_i_other}+i))
done

ps -o "pid,ppid,pcpu,pmem,rss,vsz,ucmd" -p $(cat netrhost.pid) > /dev/null 2>&1
if [ $? -eq 0 ]
then
   ps -o "pid,ppid,pcpu,pmem,rss,vsz,ucmd" -p $(cat netrhost.pid)
   echo ""
fi
echo "Process mem(Kb)    Shared Library mem(kb)   Other mem(kb)   Total(kb)"
((gl_tot=${gl_i_anon}+${gl_i_so}+${gl_i_other}))
((gl_m_anon=${gl_i_anon} / 1000))
((gl_m_so=${gl_i_so} / 1000))
((gl_m_other=${gl_i_other} / 1000))
((gl_m_tot=${gl_tot} / 1000))
gl_one="${gl_i_anon} (${gl_m_anon}M)"
gl_two="${gl_i_so} (${gl_m_so}M)"
gl_twob="${gl_i_other} (${gl_m_other}M)"
gl_three="${gl_tot} (${gl_m_tot}M)"
printf "%-18s %-24s %-15s %s\n" "${gl_one}" "${gl_two}" "${gl_twob}" "${gl_three}"
