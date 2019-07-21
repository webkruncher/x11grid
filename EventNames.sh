
IFS=$'\n'
for item in `cat EventNames.txt | tr '\t' ' ' | cut -d ' ' -f2`; do
	echo -ne "\t\t\t";
	echo -ne "case ${item}: cout << \"${item}\" << endl; break;\n"
done;

