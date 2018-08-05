#include <WiFi.h>
#include <WiFiClient.h>

// ssid and password for connecting to Wi-Fi router
const char *ssid_router = "";
const char *password_router = "";

// ssid and password for an original access point
const char *ssid_ap = "ESP32ap";
const char *password_ap = "12345678";

unsigned long start_time_millis = 0;

// Time when the bell rings.
int ring_seconds[3] = {0, 0, 0};

unsigned long suspension_start_time = 0;  // Time when stops timer[msec].
unsigned long total_suspension_time = 0;  // Total elapsed time while timer stops[msec].
bool is_stopping = false; // Is the timer stops?
int elapsed_time = 0; // Elapsed time

int elapsed_time_mng[2] = {0, 0}; // [is_stopping, Elapsed time]

// TCP server at port 80 will respond to HTTP requests
WiFiServer server(80);

void setup(){
  Serial.begin(115200);
  Serial.println("----------BEGIN SETUP----------");

  // Start access point
  // Serial.println();
  // Serial.print("Configuring access point...");
  // WiFi.softAP(ssid, password);
  // IPAddress myIP = WiFi.softAPIP();
  // Serial.print("AP IP address: ");
  // Serial.println(myIP);

  // Connect to router
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid_router);
  WiFi.begin(ssid_router, password_router);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Start Web Server server
  server.begin();
  Serial.println("Started Web server");

  Serial.println("----------END SETUP----------");
}

void loop(){
  communication();
}

void communication(){
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client){
    return;
  }

  String responce = "";
  Serial.println("New client");
  while(client.connected()) {
    // Client is connected

    // Check if the client sends request
    if (client.available()) {
      String req = client.readStringUntil('\r');
      client.read(); // Read "\n"
      Serial.println(req);
      // Parse the request to see which page the client want
      int addr_start = req.indexOf(' ');
      int addr_end = req.indexOf(' ', addr_start + 1);
      if (addr_start == -1 || addr_end == -1) {
          Serial.print("Invalid request: ");
          Serial.println(req);
          return;
      }

      req = req.substring(addr_start + 1, addr_end);
      Serial.print("Request: ");
      Serial.println(req);

      if(req.indexOf("/") != -1) {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println();

        client.println("<!DOCTYPE html>");
        client.println("<html>");
        client.println("<head>");
        client.println("<meta name='viewport' content='initial-scale=1.5'>");
        client.println("</head>");
        client.println("<body>");
        client.println("<form method='get'>");
        client.println("<font size='4'>Timer Example<br>");
        client.println("<br>");

        // Change output depends on a request
        if(req.indexOf("Get") != -1){
          client.print("Time : " + printTime(millis() - start_time_millis) + "<br>");
        }else if(req.indexOf("Reset") != -1){
          start_time_millis = millis();
          client.println("Restart timer.<br/>");
        }else if(req.indexOf("Set") != -1){
          client.println("Ringing Time<br>");
          for(int i=0; i<3; i++){
            int m_start = req.indexOf("m" + String(i));
            int m_end = req.indexOf("&", m_start);
            int min = (int)req.substring(m_start + 3, m_end).toInt();
            int s_start = req.indexOf("s" + String(i));
            int s_end = req.indexOf("&", s_start);
            int sec = (int)req.substring(s_start + 3, s_end).toInt();
            ring_seconds[i] = min * 60 + sec;
            client.print("[" + String(i) + "] : " + printTime(ring_seconds[i]) + "<br>");
          }

        }

        client.println("</font><br>");
        client.println("<input type='submit' name='Get' value='Get' style='background-color:black; color:white;'>");
        client.println("<input type='submit' name='Reset' value='Reset' style='background-color:black; color:white;'>");
        client.println("<input type='submit' name='Set' value='Set' style='background-color:black; color:white;'>");
        client.println("<br><br>");
        client.println("Set ringing times<br>");
        client.print("[0] : ");
        client.print("<input type='number' name=m0 value='0' max='59' min='0' >");
        client.print(":");
        client.println("<input type='number' name=s0 value='0' max='59' min='0' ><br>");
        client.print("[1] : ");
        client.print("<input type='number' name=m1 value='0' max='59' min='0' >");
        client.print(":");
        client.println("<input type='number' name=s1 value='0' max='59' min='0' ><br>");
        client.print("[2] : ");
        client.print("<input type='number' name=m2 value='0' max='59' min='0' >");
        client.print(":");
        client.println("<input type='number' name=s2 value='0' max='59' min='0' ><br>");
        client.println("</form>");
        client.println("</body>");
        client.println("</html>");

        client.println();
      }else{
        // if we can not find the page that client request then we return 404 File not found
        client.println("HTTP/1.1 404 Not Found");
        Serial.println("Sending 404");
      }
      break;
    }
  }

  client.stop();
  Serial.println("client disonnected");
  Serial.println("------------------------------");
  Serial.println("");
}

String printTime(unsigned long time_millis){
  unsigned long seconds = time_millis / 1000;
  return String(seconds / 60) + ":" + String(seconds % 60);
}

String printTime(int time_sec){
  return String(time_sec / 60) + ":" + String(time_sec % 60);
}