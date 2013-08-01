DESCRIPTION:
Allows you to view the connectivity results of multiple domains from a single webpage.


QUICK START:
* Set up a virtual host to server this project
* Navigate to the web root and delete the EMPTY web root directory (NOTE: this
  project will replace the web root in the next command)
* Download the project as the web root folder with this command in shell:

  git clone git://github.com/aklump/urls_monitor.git web_root_folder

* Make sure the directory 'config' is writable by the php user.
* To add your first page use the install.sh script in shell.

  . install.sh

ADDING ADDITIONAL PAGES:
* You may have as many additional pages as you wish, which are accessed in the
  url as if subdirectories. The first page you probably configured in
  config/default. However if you wanted to access a page located at the url:
  /website it would have it's configuration files located in /config/website.
  Likewise /website2 would have config files located at /config/website2.
* You may next pages like this: /website/all and website/some would have their
  config files located at /config/website/all and /config/website/some, which
  the config files for /website would still be located at /config/website.
* To install additional pages you run something like this from shell:

  . install.sh website
  . install.sh website/all
  . install.sh website/some

MANUAL INSTALLATION/CONFIGURATION:
* /config/default/urls.default.txt and /config/default/config.default.ini are
  provided as examples
* Copy /config/default/urls.default.txt as urls.txt and populate with all domains you wish to
  monitor. I prefer to omit the leading wwww. to make sure and test that they
  redirect as appropriate by prepending the www.
* Enter all domains, one per line into urls.txt. You may skip lines. Not that
  you may also add comments using any one of the following: '#', '//', ';' at
  the beginning of the line.
* Copy /config/default/config.default.ini as config.ini
* Create aliases for your ips if you want, in config.ini
* See /config/default/config.default.ini for lots more configuration options.

To allow tablesorting you need to download tablesorter
* http://tablesorter.com/docs/#Download
* Move jquery.tablesorter.min.js project's root folder


THEMING
* You SHOULD NOT edit /style.css, instead create a file called style.css for css
  overrides as /config/default/style.css and it will be automatically included
  for you. You may also include a style.css in each of your subpages if you wish
  as a final cascade.


USAGE:
* Ping index.php and view your results.

--------------------------------------------------------
CONTACT:
In the Loft Studios
Aaron Klump - Web Developer
PO Box 29294 Bellingham, WA 98228-1294
aim: theloft101
skype: intheloftstudios

http://www.InTheLoftStudios.com
