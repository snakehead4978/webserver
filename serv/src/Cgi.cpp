/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/23 01:01:21 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/25 12:36:18 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Cgi.hpp"
#include "ClientSocket.hpp"

#include <execinfo.h>

static void traced_close(int fd, const char *loc)
{
    if (fd == -1)
    {
        void *bt[10];
        int n = backtrace(bt, 10);
        char **syms = backtrace_symbols(bt, n);
        std::cerr << "close(-1) called from " << loc << std::endl;
        for (int i = 0; i < n; i++)
            std::cerr << syms[i] << std::endl;
        free(syms);
        return ;
    }
    close(fd);
}

#define close(fd) traced_close(fd, __FUNCTION__)

Cgi::Cgi(ClientSocket *_client, std::string &_inter, std::string _path, std::string &_port) : ACustomSocket(CGI)
{
	inter = _inter; 
	path = _path;
	port = _port;
	sock = -1;
	sockWrite = -1;
	client = _client;
	offset = 0;
	pid = -1;
	pipeIn[0] = -1;
	pipeIn[1] = -1;
	pipeOut[0] = -1;
	pipeOut[1] = -1;
	cleaned = false;
}

Cgi::~Cgi()
{
	if (pid > 0)
	{
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
	}
	if (sock != -1)
    {
        epoll_ctl(epol, EPOLL_CTL_DEL, sock, NULL);
        allSockets.erase(sock);
    }
	if (sockWrite != -1)
	{
		close(sockWrite);
		epoll_ctl(epol, EPOLL_CTL_DEL, sockWrite, 0);
		allSockets.erase(sockWrite);
	}
	if (pipeIn[0] != -1)
		close(pipeIn[0]);
	if (pipeIn[1] != -1)
		close(pipeIn[1]);
	if (pipeOut[0] != -1)
		close(pipeOut[0]);
	if (pipeOut[1] != -1)
		close(pipeOut[1]);
	if (client)
		client->delCgi();
}

static void killMyself(Cgi *cgi)
{
	delete cgi;
	exit(1);
}

int	Cgi::cgiExit(bool ex)
{
	if (!cleaned)
	{
		client->delCgi();
		for (std::map<int, ACustomSocket*>::iterator it = allSockets.begin(); it != allSockets.end(); it++)
			close(it->first);
		allSockets.clear();
		close(epol);
		client = 0;
		cleaned = true;
	}
	if (ex)
		killMyself(this);
	return (1);
}

void	Cgi::buildHeaders(std::vector<std::string> &httpHeaders)
{
	std::map<std::string, std::list<std::string> > &headers = client->getMessage()->getHeaderMap();
	if (headers.empty())
		return ;
	for (std::map<std::string, std::list<std::string> >::const_iterator it = headers.begin(); it != headers.end(); it++)
	{
		if (it->first == "content-type" || it->first == "content-length")
			continue ;
		std::string var = "HTTP_";
		for (std::string::size_type i = 0; i < it->first.size(); i++)
		{
			if (it->first[i] == '-')
				var += '_';
			else
				var += (char)toupper(it->first[i]);
		}
		var += '=';
		if (it->second.empty())
			continue ;
		for (std::list<std::string>::const_iterator v = it->second.begin(); v != it->second.end(); v++)
		{
			if (v != it->second.begin())
				var += ',';
			var += *v;
		}
		httpHeaders.push_back(var);
	}
}

static int	chdirToScript(std::string &path, std::string &scriptName)
{
	char cwd[PATH_MAX];
	if (getcwd(cwd, PATH_MAX) == NULL)
		return (1);
	std::string::size_type slash = path.rfind('/');
	scriptName = path;
	if (slash != path.npos)
	{
		std::string dir = path.substr(0, slash);
		if (dir[0] != '/')
			dir = std::string(cwd) + "/" + dir;
		if (chdir(dir.c_str()) == -1)
			return (1);
		scriptName = path.substr(slash + 1);
	}
	return (0);
}

int	Cgi::buildEnv()
{
	std::string methodEnv;
	std::string contentLengthEnv;
	std::string scriptEnv;
	std::string queryEnv;
	std::string contentTypeEnv;
	std::string requestUriEnv;
	std::string pathInfoEnv;
	std::string queryString;
	std::string portEnv;
	std::string addrEnv;
	char		*argv[3];
	std::string requestUri = client->getMessage()->getTarget();
	std::string::size_type q = requestUri.find('?');
	if (q != requestUri.npos)
		queryString = requestUri.substr(q + 1);
	std::string scriptName;
	if (chdirToScript(path, scriptName))
		return (1);
	std::vector<std::string> httpHeaders;
	buildHeaders(httpHeaders);
	methodEnv = "REQUEST_METHOD=";
	if (client->getMessage()->getMethod() & GET)
		methodEnv.append("GET");
	else if (client->getMessage()->getMethod() & POST)
		methodEnv.append("POST");
	else
		methodEnv.append("DELETE");
	char cwd[PATH_MAX];
	getcwd(cwd, PATH_MAX);
	scriptEnv 			= "SCRIPT_FILENAME=" + std::string(cwd) + "/" + scriptName;
	contentLengthEnv	= "CONTENT_LENGTH=" + convert((int)client->getBody().size());
	// scriptEnv 			= "SCRIPT_FILENAME=" + std::string() + path;
	queryEnv			= "QUERY_STRING=" + queryString;
	requestUriEnv		= "REQUEST_URI=" + requestUri;
	pathInfoEnv			= "PATH_INFO=" + requestUri.substr(0, q != requestUri.npos ? q : requestUri.size());
	portEnv 			= "SERVER_PORT=" + port;
	addrEnv 			= "SERVER_ADDR=127.0.0.1";
	try
	{
		contentTypeEnv = "CONTENT_TYPE=" + client->getMessage()->getHeader("content-type");
	}
	catch (const std::exception& e)
	{
		(void)e;
		contentTypeEnv = "CONTENT_TYPE=";
	}
	argv[0] = (char *)inter.c_str();
	argv[1] = (char *)scriptName.c_str();
	argv[2] = NULL;
	std::vector<char *> env;
	env.push_back((char *)methodEnv.c_str());
	env.push_back((char *)contentLengthEnv.c_str());
	env.push_back((char *)scriptEnv.c_str());
	env.push_back((char *)queryEnv.c_str());
	env.push_back((char *)requestUriEnv.c_str());
	env.push_back((char *)pathInfoEnv.c_str());
	env.push_back((char *)"GATEWAY_INTERFACE=CGI/1.1");
	env.push_back((char *)"SERVER_PROTOCOL=HTTP/1.1");
	env.push_back((char *)"REDIRECT_STATUS=200");
	env.push_back((char *)portEnv.c_str());
	env.push_back((char *)addrEnv.c_str());
	try
	{
		const std::list<std::string> &ctList = client->getMessage()->getHeaders("content-type");
		std::string ct;
		for (std::list<std::string>::const_iterator it = ctList.begin(); it != ctList.end(); it++)
		{
			if (!ct.empty()) ct += "; ";
			ct += *it;
		}
		contentTypeEnv = "CONTENT_TYPE=" + ct;
	}
	catch (const std::exception &e)
	{
		(void)e;
		contentTypeEnv = "CONTENT_TYPE=";
	}
	env.push_back((char *)contentTypeEnv.c_str());
	if (!httpHeaders.empty())
	{
		for (std::vector<std::string>::iterator it = httpHeaders.begin(); it != httpHeaders.end(); it++)
			env.push_back((char *)it->c_str());
	}
	env.push_back(NULL);
	execve(inter.c_str(), argv, &env[0]);
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	std::cerr << "execve failed: " << strerror(errno) << std::endl;
	return (1);
}

int	Cgi::execute()
{
	if (pipe(pipeIn) == -1 || pipe(pipeOut) == -1)
		return (std::cerr << "pipe failed\n", 500);
	pid = fork();
	if (pid == -1)
		return (std::cerr << "fork failed\n", 500);
	if (!pid)
	{
		close(pipeIn[1]);
		close(pipeOut[0]);
		if (dup2(pipeIn[0], STDIN_FILENO) == -1)
			return (-10);
		if (dup2(pipeOut[1], STDOUT_FILENO) == -1)
			return (-10);
		close(pipeIn[0]);
		close(pipeOut[1]);
		buildEnv();
		return (-10);
	}
	time = std::time(0);
	close(pipeIn[0]);
	close(pipeOut[1]);
	pipeIn[0] = -1;
	pipeOut[1] = -1;
	if (client->getMessage()->getMethod() & POST)
	{
		sockWrite = pipeIn[1];
		pipeIn[1] = -1;
		int flags = fcntl(sockWrite, F_GETFL, 0);
		if (flags == -1 || fcntl(sockWrite, F_SETFL, flags | O_NONBLOCK) == -1)
		{
			close(sockWrite);
			sockWrite = -1;
			return (std::cerr << "fcntl failed\n", 500);
		}
		struct epoll_event ev;
		ev.data.fd = sockWrite;
		ev.events = EPOLLOUT;
		if (epoll_ctl(epol, EPOLL_CTL_ADD, sockWrite, &ev))
		{
			close(sockWrite);
			sockWrite = -1;
			return (std::cerr << "epoll failed\n", 500);
		}
		allSockets[sockWrite] = this;
	}
	if (pipeIn[1] != -1)
		close(pipeIn[1]);
	pipeIn[1] = -1;
	sock = pipeOut[0];
	pipeOut[0] = -1;
	if (setNonblock())
	{
		close(sock);
		sock = -1;
		return (std::cerr << "nonblock failed\n", 500);
	}
	if (addToEpoll())
	{
		close(sock);
		sock = -1;
		return (std::cerr << "epoll failed\n", 500);
	}
	allSockets[sock] = this;
	client->turnCgi(false);
	return (0);
}

static void	normalizeCrlf(std::string &str)
{
	std::string normalized;
	for (std::string::size_type i = 0; i < str.size(); i++)
	{
		if (str[i] == '\n' && (i == 0 || str[i - 1] != '\r'))
			normalized += '\r';
		normalized += str[i];
	}
	str = normalized;
}

int	Cgi::handleRead()
{
	char	buf[OUTPUT_BUFF];
	bzero(buf, OUTPUT_BUFF);
	int n = read(sock, buf, OUTPUT_BUFF);
	if (n == -1)
	{
		client->cgiError(500);
		return (1);
	}
	std::string buff(0, OUTPUT_BUFF + 1);
	buff.clear();
	buff.assign(buf, n);
	if (!n)
	{
		int status;
		waitpid(pid, &status, 0);
		pid = -1;
		if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
		{
			client->cgiError(500);
			return (1);
		}
		client->turnCgi(true);
		if (client->getAnswer().empty())
		{
			client->cgiError(500);
			return (1);
		}
		normalizeCrlf(client->getAnswer());
		std::string statusLine = "HTTP/1.1 200 OK\r\n";
		std::string::size_type statusPos = client->getAnswer().find("Status:");
		if (statusPos != std::string::npos)
		{
			std::string::size_type end = client->getAnswer().find("\r\n", statusPos);
			std::string statusValue = client->getAnswer().substr(statusPos + 7, end - statusPos - 7);
			std::string::size_type first = statusValue.find_first_not_of(" \t");
			if (first != std::string::npos)
				statusValue = statusValue.substr(first);
			statusLine = "HTTP/1.1 " + statusValue + "\r\n";
			client->getAnswer().erase(statusPos, end - statusPos + 2);
		}
		if (client->prependAnswer(statusLine))
		{
			client->cgiError(502);
			return (1);
		}
		client->setConnection(client->getMessage()->getConnection());
		client->resetRead();
		return (1);
	}
	client->appendAnswer(buff);
	return (0);
}

int	Cgi::checkTime()
{
	static std::time_t t;
	t = std::time(0);
	if (std::difftime(t, time) >= 30)
		return (1);
	return (0);
}

void	Cgi::sendTimeout()
{
	client->cgiError(504);
}

int	Cgi::handleWrite()
{
	if (sockWrite == -1)
		return (500);
	std::string &body = client->getBody();
	int n = write(sockWrite, body.c_str() + offset, body.size() - offset);
	if (n <= 0)
		return (500);
	offset += n;
	if (offset == body.size())
	{
		epoll_ctl(epol, EPOLL_CTL_DEL, sockWrite, 0);
		allSockets.erase(sockWrite);
		close(sockWrite);
		sockWrite = -1;
	}
	return (0);	
}
