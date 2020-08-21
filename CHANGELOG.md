### v0.2.3:

New Features:

Enhancements: 
* Code refactored for ease of adding new drivers in the future

Bug Fixes:
* newlines and extra whitespace no longer affect file comparison

### v0.2.2:

New Features:

Enhancements: 

Bug Fixes:
* Memory leaks from unused fields patched

### v0.2.1:

New Features:
* Automatically adds ARPING_WAIT=8 to all CentOS ifcfg files
if it is missing. Can be toggled with -a flag.

Enhancements: 
* The type of an interface is more accurately discerned

Bug Fixes:
* VLAN interface names are now parsed properly

### v0.2.0:

New Features:
* Support added for Ubuntu <= v16.04 

Enhancements: 

Bug Fixes:

### v0.1.3:

New Features: 

Enhancements: 
* Backups are versioned and datestamped
* More verbose errors and output

Bug Fixes:
* Filters out duplicate routes
* No longer crashes when route file exists without an interface config
* Dynamic interfaces will no longer be defaulted to static on sync

### v0.1.2:

New Features: 
* Backup directory created and used

Enhancements: 
* Any ethernet based interfaces can now be synced

Bug Fixes:
* Checks for user permissions before attempting sync

### v0.1.1:

New Features: 
* Support for interface syncing on CentOS 6, 7, 8
* Verbose flag added to print out routes for each interface

Enhancements: 

Bug Fixes: 


### v0.1.0:

New Features: 
* Support for route syncing on CentOS 6, 7, 8

Enhancements: 

Bug Fixes: 
