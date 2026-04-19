import re


class OSCMessage:
    def __init__(self):
        self._destination = ""
        self._port = 0
        self._address = "/test"
        self._args = [""]
        self._types = [""]
        self.state = False

    @property
    def destination(self) -> str:
        return self._destination

    @destination.setter
    def destination(self, destination):
        self._destination = destination

    @property
    def port(self) -> int:
        return self._port

    @port.setter
    def port(self, port):
        if(int(port) > 0 and int(port) < 65535):
            self._port = int(port)

    @property
    def address(self):
        return self._address

    @address.setter
    def address(self, address: str):
        self._address = address

    @property
    def types(self):
        return self._types

    @types.setter
    def types(self, types):
        self._types = types

    @property
    def args(self):
        return self._args

    @args.setter
    def args(self, args):
        self._args = args

    def payload(self, payload):
        print(payload)
        payload = payload.replace("+", " ")
        payload = payload.replace("%2F", "/")
        regex = re.compile(" ")
        split_text = regex.split(payload)
        self._address = split_text[0]

        if len(split_text) > 1:
            self._args = []
            self._types = []
            re_int = re.compile(r"(^[1-9]+\d*$|^0$)")
            re_float = re.compile(r"(^\d+\.\d+$|^\.\d+$)")
            for element in split_text[1:]:
                self._args.append(element)
                if re.match(re_int, element):
                    self._types.append("i")
                elif re.match(re_float, element):
                    self._types.append("f")
                else:
                    self._types.append("s")
                '''if "." not in element:
                    self._types.append("i")
                else:
                    self._types.append("f")'''
        else:
            self._args = [""]
            self._types = [""]

    def payload_no_types(self) -> str:
        if self._args[0] is not "":
            return self.address + " " + " ".join(self._args)
        else:
            return self.address
