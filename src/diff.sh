for i in baseServer dpsio icomm
do
  cd $i
  for j in `ls *.cc *.h`
  do
    echo $i $j
    diff $j /tmp/src/$i/$j
  done
  cd ..
done
