*This project has been created as part of the 42 curriculum by <Dakojic>, <Jaqin>, <Jla-chon>.*

## Description

- Webserv as the name implies is a project with the goal of creating a webserver. A well-known webserver application is [Nginx](https://nginx.org/) which we will attempt a simplified recreation of its HTTP web server functionality. So first off what does a HTTP web server do?
	- A server is a socket (fd) that listens on a specific port and ip address (in our case only localhost).
	- The socket establishes a two-way connection with clients who asks for the specific address:port combination.
	- The connection is a more complex pipe that both parties can read and write to.
	- The server has to accept only HTTP requests as per the [RFC](https://www.rfc-editor.org/standards "Documentation").
	- The server will then treat the request and send back an HTTP response with the appropriate resource.
- The main server has to be written in C++, being the first big project written in C++ instead of C, we encountered the upside and downsides of a higher level programming language, with some of those including:
	- **Classes** - The use of classes is a big difference compared to the less practical structs in C.
	- **Memory** - The use of new and delete facilitates memory management in most cases, but also complicates if combined with the use of fork() and exit().
	- **STL** - The Standard Template Library, part of the C++ standard library introduces some very useful containers that are already optimised. No need to write our own linked list everytime!
- The goal of the project is to help us understand how the internet functions, through the lens of the middleman, the web server. It is the bridge between the clients and the folders containing web pages.

## Instructions

- **Compilation:** Run `make` at the root of the repository to build the project.
- **Usage:** Start the server with `./webserv [conf]` with *conf* being an optional configuration file that resembles an NGINX configuration file.
- **Clean:** Use `make clean` to remove object files, or `make fclean` to remove everything including the program.

## Resources

- [cppreference.com](https://en.cppreference.com) — Reference for standard C++ library, very useful to keep it on a separate screen to check whether a function is part of the standard library and also its relevant version. We are only allowed to use functions part of the C++98 standard and not later.
- [RFC](https://www.rfc-editor.org/standards) - Internet standards that HTTP versions use to make communication between servers easier on the web.
- [MDN](https://developer.mozilla.org/en-US/docs/Web) - Mozilla's personal documentation on an simpler and easier to read version of the web, good for a quick overview of how web servers work.
- **AI usage:** Claude was used to check over small coding errors and debugging. No code was copy-pasted, all suggestions were taken as such.