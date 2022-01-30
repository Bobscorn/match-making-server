#pragma once

#include <string>
#include <queue>
#include <unordered_map>

#include "ptop_socket.h"
#include "interfaces.h"


#ifdef WIN32
#pragma warning(push)
#pragma warning(disable : 4250)
#endif

readable_ip_info convert_to_readable(const raw_name_data&);

class Platform : public virtual ISocketWrapper {    
    protected:    
	PtopSocket _socket;
	std::string _address;
	std::string _port;
	std::string _endpoint_address;
	std::string _endpoint_port;
	bool _endpoint_assigned = false;

	void try_update_name_info();
	void try_update_endpoint_info();
	void update_name_info();
	void update_endpoint_info();
	void update_endpoint_if_needed();

	public:
    Platform(PtopSocket&& socket);

	readable_ip_info get_peer_data() const override;
	raw_name_data get_peername_raw() const override;
	raw_name_data get_myname_raw() const override;
	inline readable_ip_info get_peername_readable() const override { return convert_to_readable(get_peername_raw()); }
	inline readable_ip_info get_myname_readable() const override { return convert_to_readable(get_myname_raw()); };

	inline std::string get_my_ip() const override { return _address; }
	inline std::string get_my_port() const override { return _port; }
	inline std::string get_endpoint_ip() const override { return _endpoint_address; }
	inline std::string get_endpoint_port() const override { return _endpoint_port; }	
	std::string get_identifier_str() override;

	inline const std::string& get_name() const override { return _socket.get_name(); }
	inline void set_name(std::string name) override { return _socket.set_name(name); }

	inline PtopSocket&& release_socket() { return std::move(_socket); }
    virtual ~Platform();
};

class PlatformListener : public Platform, public virtual IListenSocketWrapper {
	public:
	PlatformListener(std::string port, protocol input_protocol, std::string name);
 	void listen() override;
 	bool has_connection() override;
	std::unique_ptr<IDataSocketWrapper> accept_connection() override;
};

class PlatformAnalyser : public Platform, public virtual IDataSocketWrapper {
	std::queue<Message> _stored_messages;
	void process_socket_data();

	public:
	PlatformAnalyser(std::unique_ptr<INonBlockingConnector>&& old);
	PlatformAnalyser(PtopSocket&& socket);
	PlatformAnalyser(std::string peer_address, std::string peer_port, protocol input_protocol, std::string name);

	Message receive_message() override;
	bool has_message() override;

	bool send_data(const Message& message) override;
};

class NonBlockingListener : public Platform, public virtual INonBlockingListener {
	public:
	NonBlockingListener(raw_name_data data, protocol input_protocol, std::string name);

	void listen() override;
	bool has_connection() override;
	std::unique_ptr<IDataSocketWrapper> accept_connection() override;
};

class NonBlockingConnector : public Platform, public virtual INonBlockingConnector {
	public:
	NonBlockingConnector(raw_name_data private_binding, std::string ip_address, std::string port, protocol input_protocol, std::string);

	void connect(std::string ip_address, std::string port) override; // Called in constructor, can be called again if it fails
	ConnectionStatus has_connected() override;
};

// Taken from https://stackoverflow.com/a/19195373
template<class T>
inline void hash_combine(std::size_t& s, const T& v)
{
	std::hash<T> h;
	s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

namespace std
{
	template<>
	struct hash<raw_name_data>
	{
		std::size_t operator()(const raw_name_data& name_data) const
		{
			using std::size_t;
			using std::hash;
			using std::string;
			
			auto readable = convert_to_readable(name_data);

			size_t h = hash<string>()(readable.ip_address);
			hash_combine(h, readable.port);
			return h;
		}
	};
}


class UDPListener;

class UDPAcceptedConnector : public virtual IDataSocketWrapper
{
	void throw_if_no_listener() const;

	UDPListener* _listen;
	raw_name_data _my_endpoint;

	friend class UDPListener;

	UDPAcceptedConnector(UDPListener* listen, raw_name_data endpoint);

public:
	~UDPAcceptedConnector();

	Message receive_message() override;
	bool has_message() override;

	bool send_data(const Message& message) override;

	// Inherited via IDataSocketWrapper
	virtual readable_ip_info get_peer_data() const override;
	virtual raw_name_data get_peername_raw() const override;
	virtual raw_name_data get_myname_raw() const override;
	virtual readable_ip_info get_peername_readable() const override;
	virtual readable_ip_info get_myname_readable() const override;
	virtual std::string get_my_ip() const override;
	virtual std::string get_my_port() const override;
	virtual std::string get_endpoint_ip() const override;
	virtual std::string get_endpoint_port() const override;
	virtual std::string get_identifier_str() override;
	virtual const std::string& get_name() const override;
	virtual void set_name(std::string name) override;
};

class UDPListener : public Platform, public virtual IListenSocketWrapper
{
	std::unordered_map<raw_name_data, std::queue<Message>> _messages;
	std::unordered_map<raw_name_data, UDPAcceptedConnector*> _connectors;
	std::vector<raw_name_data> _new_connections;

	friend class UDPAcceptedConnector;

	void process_data();

	void remove_connector(raw_name_data endpoint, UDPAcceptedConnector* conn);

	bool send_data(const Message& message, raw_name_data to);
	bool has_message(raw_name_data from);
	Message receive_message(raw_name_data from);

	raw_name_data my_data();
public:
	UDPListener(std::string port, protocol proto, std::string name);

	inline void listen() override {}
	bool has_connection() override;
	std::unique_ptr<IDataSocketWrapper> accept_connection() override;
};

#ifdef WIN32
#pragma warning(pop)
#endif