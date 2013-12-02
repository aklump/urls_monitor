# This is an update to 0.7 which creates the themes folder in config and moves
# all pages into a subfolder called pages
if [[ ! -d config/pages ]]; then
  mv config pages
  mkdir -p config/themes
  mv pages config/
fi
if [[ -d config/pages ]]; then
  echo "`tput setaf 2`Update successful.`tput op`"
else
  echo "`tput setaf 1`update failed.`tput op`"
fi