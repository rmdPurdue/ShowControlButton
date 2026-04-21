

class NetworkConfig:

    def __init__(self):
        self._ip = ()
        self._subnet = ()
        self._gateway = ()
        self._dns = ()
        self.disconnected = False

    @property
    def ip(self):
        return self._ip

    @ip.setter
    def ip(self, ip):
        self._ip = list(map(int, ip.split(".")))

    @property
    def subnet(self):
        return self._subnet

    @subnet.setter
    def subnet(self, subnet):
        self._subnet = list(map(int, subnet.split(".")))

    @property
    def gateway(self):
        return self._gateway

    @gateway.setter
    def gateway(self, gateway):
        self._gateway = list(map(int, gateway.split(".")))

    @property
    def dns(self):
        return self._dns

    @dns.setter
    def dns(self, dns):
        self._dns = list(map(int, dns.split(".")))

    def ip_as_string(self) -> str:
        return ".".join(map(str, self._ip))

    def subnet_as_string(self) -> str:
        return ".".join(map(str, self._subnet))

    def dns_as_string(self) -> str:
        return ".".join(map(str, self._dns))

    def gateway_as_string(self) -> str:
        return ".".join(map(str, self._gateway))