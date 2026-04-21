network = None
ip = None
gateway = None
subnet = None
dns = None
checked = None
destinationIP = None
destinationPort= None
message = None

HTML_FORM = """
<!DOCTYPE html>
<html lang="en">

<head>
  <meta charset="UTF-8">
  <title>Network Control Panel</title>
  <style>
    body {{
      background-color: #1e1e1e;
      font-family: "Segoe UI", Tahoma, sans-serif;
      color: #e0e0e0;
      display: flex;
      flex-direction: column;
      align-items: center;
      min-height: 100vh;
      margin: 0;
      padding: 50px;
      box-sizing: border-box;
      gap: 30px;
    }}

    .panel {{
      background-color: #2b2b2b;
      padding: 20px 25px 30px;
      border-radius: 8px;
      box-shadow: 0 0 15px rgba(0,0,0,0.6);
      width: 420px;
    }}

    .panel h2 {{
      margin-top: 0;
      text-align: center;
      color: #ffffff;
      letter-spacing: 1px;
    }}

    .section {{
      margin-bottom: 20px;
      padding: 15px;
      border: 1px solid #444;
      border-radius: 6px;
      background-color: #252525;
    }}

    .section-title {{
      font-size: 14px;
      margin-bottom: 10px;
      color: #9cdcfe;
      text-transform: uppercase;
      letter-spacing: 1px;
    }}

    label {{
      display: block;
      font-size: 13px;
      margin-top: 10px;
    }}

    p.note {{
      font-style: italic;
      font-size: 11px;
    }}

    p.description {{
      font-size: 12px;
    }}

    p.alert {{
      font-size: 12px;
      color: red;
    }}

    input[type="text"],
    input[type="number"] {{
      width: 90%;
      padding: 6px 8px;
      margin-top: 4px;
      border-radius: 4px;
      border: 1px solid #555;
      background-color: #1e1e1e;
      color: #fff;
    }}

    input:focus {{
      outline: none;
      border-color: #007acc;
    }}

    input:invalid {{
      border-color: #ff5555;
    }}

    input:valid {{
      border-color: #4caf50;
    }}

    .toggle {{
      display: flex;
      align-items: center;
      margin-top: 10px;
      cursor: pointer;
    }}

    .toggle input {{
      margin-right: 8px;
      transform: scale(1.2);
    }}

    button {{
      width: 100%;
      padding: 10px;
      border: none;
      border-radius: 5px;
      background-color: #007acc;
      color: white;
      font-size: 14px;
      cursor: pointer;
      margin-top: 10px;
    }}

    button:hover {{
      background-color: #0094ff;
    }}
  </style>
</head>
<body>

  <!-- Form 1: Network Settings -->
  <form class="panel" method="post" action="/submit-network">
    <h2>Network Settings</h2>
    <p class="description">Configure the IP address, subnet mask, gateway, and DNS server for this device.</p>

    <div class="section">
      <div class="section-title">Network Configuration</div>

      <p class="alert" style="{network}">Network settings updated.</p>

      <label>
        IP Address
        <input type="text" name="ip" required
          pattern="^((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)\.){{3}}(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)$"
          title="Enter a valid IPv4 address (e.g., 192.168.1.100)"
          value="{ip}">
      </label>

      <label>
        Subnet Mask
        <input type="text" name="subnet" required
          pattern="^((255|254|252|248|240|224|192|128|0)\.){{3}}(255|254|252|248|240|224|192|128|0)$"
          title="Enter a valid subnet mask (e.g., 255.255.255.0)"
          value="{subnet}">
      </label>

      <label>
        Gateway
        <input type="text" name="gateway" required
          pattern="^((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)\.){{3}}(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)$"
          title="Enter a valid IPv4 address"
          value="{gateway}">
      </label>

      <label>
        DNS Server
        <input type="text" name="dns" required
          pattern="^((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)\.){{3}}(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)$"
          title="Enter a valid IPv4 address"
          value="{dns}">
      </label>
    </div>

    <button type="submit">Apply Network Settings</button>
  </form>

  <!-- Form 2: OSC Message Settings -->
  <form class="panel" method="post" action="/submit-osc">
    <h2>OSC Message Settings</h2>
    <p class="description">Configure an OSC message to send alongside the contact closure output when the Show Button is pressed.</p>

    <div class="section">
      <div class="section-title">OSC Configuration</div>

      <label class="toggle">
        <input type="checkbox" id="oscToggle" name="sendOSC" {checked}>
        Enable OSC Message
      </label>
      <p class="note">Note: if this box is unchecked, only the contact closure output will be enabled.</p>

      <p class="alert" style="{osc}">OSC settings updated.</p>

      <label>
        Destination IP Address
        <input type="text" name="destIp"
          pattern="^((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)\.){{3}}(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)$"
          title="Enter a valid IPv4 address"
          value="{destinationIP}">
      </label>

      <label>
        Destination Port
        <input type="number" name="destPort" min="1" max="65535"
          title="Valid port range: 1–65535"
          value="{destinationPort}">
      </label>

      <label>
        Message String
        <input type="text" name="message"
          value="{message}">
      </label>
    </div>

    <button type="submit">Apply OSC Settings</button>
  </form>

</body>
</html>
"""
