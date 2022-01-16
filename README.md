# Website Censorship Proxy
A proxy that can a browser can connect to and it will block a website if it contains a keyword to be blocked. Only works with HTTP.
First connects to the browser and receives a HTTP GET request from it.
Then it will connect to the web server in the request and sends the HTTP GET request and scans what is returned for the keywords it has stored.
If a keyword is found it sends a error screen.
  
The port number of the proxy can be changed by changing the definition of PROXYPORTNUM
  
Blocks pages that contain certain keywords. By default "Floppy" and "SpongeBob" are keywords  
that are blocked.  
  
Can telnet into proxy to dynamically add and remove keywords.  
KEYWORDA+ adds a the word typed after the + to the list of blocked words.  
KEYWORDD+ deletes the word typed after the + from the list of blocked words.  
EXIT to close the connect. Needs to be done because there are no forks.  
