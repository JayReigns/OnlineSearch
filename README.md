# Searching Internet using Sockets in C

*You can search a Query on Internet using this apllicaton.*

*Ex. You can search for example ‘python’ and the program will print related results.*

### This project uses DuckDuckGo Instant Search API to fetch query results as XML data.
[http://api.duckduckgo.com/?q=DuckDuckGo&format=xml](http://api.duckduckgo.com/?q=DuckDuckGo&format=xml)

### Compile:

- for POSIX :
  `gcc search.c -o search`

- for Windows:
  `gcc search.c -lWs2_32 -o search`
