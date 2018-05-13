#!/bin/bash
echo Welcome to Web Creator
set -o nounset
#set -o errexit

#echo arg1: $1 arg2: $2 arg3: $3 arg4: $4

#echo $#
if [[ $# -ne 4 ]]; then
	echo "error: Usage ./webcreator.sh root_directory text_file w p" >&2; exit 1;
fi

root_directory="$1"
text_file="$2"
w="$3"
p="$4"

#check if directory and file exist
if [ ! -d "$root_directory" ]; then echo "error: Directory $root_directory not found." >&2; exit 1; fi
if [ ! -r "$text_file" ]; then echo "error: File $text_file not found." >&2; exit 1; fi

#check if arguements w, p are numbers
re='^[0-9]+$'
if ! [[ $w =~ $re ]] ; then echo "error: Value $w not a number." >&2; exit 1; fi
if ! [[ $p =~ $re ]] ; then echo "error: Value $p not a number." >&2; exit 1; fi

#check number of lines of text_file
numOfLines=$(wc -l < "$text_file")
limit=10000
#echo numOfLines: $numOfLines
if [ "$numOfLines" -lt "$limit" ]; then echo "error: Not enough lines." >&2; exit 1; fi

#check if there are files in root directory
if [ "$(ls -A $root_directory)" ]; then
	echo "# Warning: directory is full, purging ..."
	rm -rf $root_directory/*
fi

declare -A sites
declare -A haslink

RANDOM=1

for (( i = 0; i < $w; i++ )); do
	mkdir $root_directory/site$i
	echo "# Creating web site $i ..."
	for (( j = 0; j < $p; j++ )); do
		while : ; do
			sites[$i,$j]=$root_directory/site$i/page${i}_$RANDOM.html
			[[ -r ${sites[$i,$j]} ]] || break
		done

		touch ${sites[$i,$j]}
	done
done

for key in "${!sites[@]}"
do
	k=$(($RANDOM % ($numOfLines - 2000-2)+2))
	m=$(($RANDOM % (1000-1)+1001))
	f=$((($p / 2) + 1))
	q=$((($w / 2) + 1))

	linkAfterLines=$(($m/($f+$q)))
	echo "# Creating page ${sites[$key]} with $m lines starting at line $k ..."
	cat > ${sites[$key]} << ASDF
<!DOCTYPE html>
<html>
<body>
ASDF
	i=$(echo $key | cut -d ',' -f 1)
	lines=$(head -$((k+m-1)) $text_file | tail -$m)
	count=1
	IFS=$'\n'       # make newlines the only separator
	for line in $lines
	do
		echo $line >> ${sites[$key]}
		if [[ $count -eq $linkAfterLines ]]; then
			if [[ $f -ne 0 ]] ; then
				site=${sites[$i,$(($RANDOM%$p))]}
				haslink[$site]=1
				echo "<a href=../../$site>Internal link</a>" >> ${sites[$key]}
				echo "# Adding link to $site"
				((f--))
			else
				while : ; do
					j=$(($RANDOM%$w))
					[[ $i -eq $j ]] || break
				done
				site=${sites[$j,$(($RANDOM%$p))]}
				haslink[$site]=1
				echo "<a href=../../$site>External link</a>" >> ${sites[$key]}
				echo "# Adding link to $site"
			fi

			count=0
		fi
		((count++))
	done


	cat >> ${sites[$key]} << ASDF
</body>
</html>
ASDF
done

if [[ ${#haslink[@]} -eq $(($w*$p)) ]]; then
	echo "# All pages have at least one incoming link"
else
	echo "# Not all pages have at least one incoming link"
fi
echo "# Done."
