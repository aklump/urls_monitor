# Gleen the page name
if [ $# -ne 1 ]
then
  echo "`tput setaf 1`Please provide a project name as the argument.`tput op`"
  exit
else
  page=$1
fi

if [[ ! -d "config/pages/$page" ]]; then
  echo "`tput setaf 1`The project '$page' is not found.`tput op`"
  exit
fi

php meta.php "$page"

if [[ -f "config/pages/$page/meta.yaml" ]]; then
  echo "`tput setaf 2`$page/meta.yaml has been updated.`tput op`"
else
  echo  "`tput setaf 1`An error occurred.`tput op`"
fi