##Description
Allows you to view the connectivity results of multiple domains from a single webpage.


##Quick Start
* Set up a virtual host to serve this project
* Navigate to the web root and delete the EMPTY web root directory (NOTE: this
  project will replace the web root in the next command)
* Download the project as the web root folder with this command in shell:

      git clone git://github.com/aklump/urls_monitor.git web_root_folder

* Make sure the directory 'config' is writable by the php user.
* To add your first page use the install.sh script in shell.

      . install.sh

##Adding Additional Pages
* You may have as many additional pages as you wish, which are accessed in the url as if subdirectories. The first page you probably configured in `config/default`. However if you wanted to access a page located at the url: `/website` it would have it's configuration files located in `/config/website`.  Likewise `/website2` would have config files located at `/config/website2`.
* You may next pages like this: `/website/all` and `website/some` would have their config files located at `/config/website/all` and `/config/website/some`, which the config files for `/website` would still be located at `/config/website`.
* To install additional pages you run something like this from shell:

        . install.sh website
        . install.sh website/all
        . install.sh website/some
      
* The only required config file--when creating child pages--is `urls.txt`.  That is to say the other configuration files will cascade down from the nearest parent if not included in the child directory.  This is especially useful for `config.ini` so you don't have duplicate work.

## Adding Meta Data
* You may provide additional arbitrary (meta) data for each url, such as an extra column for the codebase of the website.
* You do this with a file called `meta.yaml`, which is in yaml format.
* Column names <strong>must not contain spaces</strong>; however, you may use an alias with a space, if you'd like.
* To generate the template file use `meta.sh`; you may also call this after adding new urls so they populate in your template.  The following will generate or update (sort alphabetically) `config/demo/meta.yaml` using `urls.txt`.  **Note that any data already in `meta.yaml` is preserved during this step**, only new urls are added, if any.
      
        ./meta.sh demo

* Now edit `meta.yaml` and add other data as desired.  Note a conflict by key will result in the dynamic value replacing what is hardcoded in `meta.yaml`.  Be sure to wrap string values in quotes to avoid parse errors.
* The keys you provide in the yaml file will become the column keys so make sure they appear just like other columns in `config.ini`, for both visibility and ordering.

## Comment Entries
You may create an "url" which is not an url at all but a row serving as a comment.  The dynamic fields are disabled on comment rows.  Here's what to do:

1. Add a line (the comment title) to `urls.txt` that will be used in the url column.  E.g., `Shared Dropbox`
2. Now add to `meta.yaml` something like the following.  The key here is `_type: comment`

        -
            url: 'Shared Dropbox'
            _type: comment
            ftpp: changeme
            ftpu: joeuser
            ip: 192.168.0.1
3. Notice too that in this case `ip`, which is normally a dynamic field, is hardcoded and will display as such, since this is a comment.
4. Also <strong>you cannot have spaces in key names</strong>, so you may want to use an alias like this in `config.ini`

        alias[ftpp] = ftp pass
        alias[ftpu] = ftp user
  

##Manual Installation / Configuration
* `/config/default/urls.default.txt` and `/config/default/config.default.ini` are
  provided as examples
* Copy `/config/default/urls.default.txt` as `urls.txt` and populate with all domains you wish to monitor. I prefer to omit the leading www. to make sure and test that they redirect as appropriate by prepending the www.
* Enter all domains, one per line into urls.txt. You may skip lines. Not that you may also add comments using any one of the following: `#`, `//`, `;` at the beginning of the line.
* Copy `/config/default/config.default.ini` as `config.ini`
* Create aliases for your ips if you want, in `config.ini`
* See `/config/default/config.default.ini` for lots more configuration options.

To allow tablesorting you need to download tablesorter
* <http://tablesorter.com/docs/#Download>
* Move `jquery.tablesorter.min.js` project's root folder


##Export
The export features are handled by libraries that can be downloaded using composer, if composer is installed on your system.  The install script will attempt to run `composer install`.

##Theming
* Look for themes in `/themes`
* To select the theme you need to add something like this to `config.ini`
        
        theme = 'another_theme';

* You may also simply override a theme's css by adding `style.css` to your config directory.
* When building a theme if you append `#demo` to the url, you will be able to theme your ajax loading.  To see what I mean do somethign like this:

        http://monitor.local/theme_demo#demo


##Usage
* Ping `index.php` and view your results.


##Contact
* **In the Loft Studios**
* Aaron Klump - Developer
* PO Box 29294 Bellingham, WA 98228-1294
* _aim_: theloft101
* _skype_: intheloftstudios
* _d.o_: aklump
* <http://www.InTheLoftStudios.com>


http://www.InTheLoftStudios.com
