Description:
Allows you to view the connectivity results of multiple domains from a single webpage.

To allow tablesorting you need to download tablesorter
* http://tablesorter.com/docs/#Download
* Move jquery.tablesorter.min.js into this folder

Installation:
* Do the following for fast installation

* Navigate to a parent folder
* Download the project into the parent folder with this:

  git clone git://github.com/aklump/urls_monitor.git [project name]

Automatic Installation
* A script, install.sh has been provided which copies automates the steps below,
  you may run it or do it manually.

  . install.sh

Manual Configuration:
* You SHOULD NOT edit style.css, instead create a file called custom.css for css
  overrides and it will be automatically included for you.
* urls.example.txt and config.example.ini are provided as examples
* Create a file called urls.txt
* Enter all domains you want to monitor, one per line into urls.txt. You may
  skip lines. Not that you may also add comments using any one of the following:
  '#', '//', ';' at the beginning of the line.
* Create a file called config.ini
* Create aliases for your ips if you want, in config.ini
* See config.example.ini for more configuration options

Post Installation
* Delete install.sh if you see it.

  rm install.sh


Usage:
* Ping index.php and view your results

--------------------------------------------------------
CONTACT:
In the Loft Studios
Aaron Klump - Web Developer
PO Box 29294 Bellingham, WA 98228-1294
aim: theloft101
skype: intheloftstudios

http://www.InTheLoftStudios.com
