# Quick Viva Questions & Answers
## CS558: Computer Systems Lab - Assignment 3
### Group 25

## Networking Fundamentals

**Q: What is Wireshark and why did you use it for this assignment?**
A: Wireshark is a network protocol analyzer that captures and inspects data packets. We used it to capture and analyze Hotstar streaming traffic to understand protocols, performance, and content delivery patterns.

**Q: Define a packet.**
A: A packet is the fundamental unit of data transmitted over a network, containing both the data being transmitted and control information such as source and destination addresses.

**Q: What's the difference between OSI and TCP/IP models?**
A: OSI has 7 layers (Physical, Data Link, Network, Transport, Session, Presentation, Application) while TCP/IP has 4 layers (Network Interface, Internet, Transport, Application). TCP/IP is more widely implemented.

## Protocol Questions

**Q: Name the main transport layer protocols used by Hotstar.**
A: TCP for reliable delivery of video segments and HTTP requests; UDP occasionally for real-time content and within QUIC protocol.

**Q: Why is HTTPS preferred over HTTP for streaming?**
A: HTTPS provides encryption to protect content from piracy, ensures user privacy, maintains data integrity, and prevents ISP interference or throttling.

**Q: What is adaptive bitrate streaming?**
A: A technique where content is encoded at multiple quality levels, allowing clients to switch between them based on network conditions to optimize playback quality while minimizing buffering.

**Q: What are manifest files in streaming?**
A: Files (like .m3u8 or .mpd) that list available video qualities, segment URLs, durations, and other metadata necessary for the player to request appropriate content.

**Q: What is a CDN and why is it important for streaming?**
A: A Content Delivery Network is a geographically distributed network of servers that delivers content from locations closer to users, reducing latency and improving streaming performance.

**Q: What HTTP status code is most common for video segment delivery?**
A: 206 Partial Content, which indicates successful delivery of a requested range of bytes from a resource.

## Performance Metrics

**Q: How do you measure throughput in network analysis?**
A: By calculating the amount of data transferred (in bits) divided by the time interval (in seconds), typically expressed as Mbps (Megabits per second).

**Q: What is RTT and why is it important for streaming?**
A: Round Trip Time is the time taken for a packet to travel from source to destination and back. Lower RTT improves streaming responsiveness, especially for interactive features.

**Q: What causes packet loss and how does it affect streaming?**
A: Network congestion, hardware failures, or signal interference cause packet loss, which can lead to video buffering, quality drops, or playback interruptions.

**Q: Why does Hotstar's performance vary at different times of day?**
A: Due to varying network congestion levels, with evening hours (6-10 PM) showing higher usage, increased RTT, and lower available bandwidth due to peak internet usage.

## Technical Implementation

**Q: What happens when a user seeks to a different position in a video?**
A: The player consults the manifest, requests the specific segment containing the target position, clears unnecessary buffered content, and issues accelerated requests to build a new buffer.

**Q: How does Hotstar implement multi-CDN strategy?**
A: By distributing content across multiple providers (Akamai, Amazon CloudFront, etc.) for redundancy, geographic optimization, load balancing, and cost efficiency.

**Q: What occurs during a TCP three-way handshake?**
A: Client sends SYN, server responds with SYN-ACK, client acknowledges with ACK, establishing a connection before data transfer begins.

**Q: How do TLS handshakes affect streaming startup time?**
A: TLS handshakes add initial latency (typically 50-300ms) as client and server exchange cipher information and establish encrypted connection before content delivery begins.

**Q: What happens to network traffic when a video is paused?**
A: The client stops requesting new segments, maintains periodic heartbeats to keep the session alive, sends analytics data, and occasionally probes bandwidth.

**Q: Why does Hotstar use both HLS and DASH protocols?**
A: For device compatibility (HLS for iOS/Safari, DASH for others) and to leverage the specific advantages of each protocol in different viewing scenarios.

## Analysis Methods

**Q: How did you filter Hotstar traffic in Wireshark?**
A: Using display filters like "host contains hotstar" and "http.request.uri contains .ts or .m4s or .mp4" to isolate relevant traffic.

**Q: How did you identify different CDN providers in your analysis?**
A: Through reverse DNS lookups, WHOIS database queries, examining TLS certificates, and analyzing response headers for CDN-specific patterns.

**Q: What is the significance of TCP Window Size in streaming performance?**
A: It controls flow rate between sender and receiver; larger window sizes allow more data to be sent before requiring acknowledgment, improving streaming throughput.

**Q: How can you determine if a streaming service is using adaptive bitrate?**
A: By observing changes in requested segment qualities over time, examining manifest files listing multiple bitrates, and monitoring throughput fluctuations.

**Q: What metrics indicate efficient streaming implementation?**
A: Low initial buffering time, minimal rebuffering events, appropriate quality adaptation to bandwidth changes, low retransmission rates, and consistent throughput.

**Q: How does geographic server distribution affect streaming performance?**
A: Servers closer to users provide lower latency, better throughput, and more reliable connections, resulting in faster startup and fewer interruptions.

## Wireshark Tools & Features

**Q: What is the Protocol Hierarchy Statistics tool in Wireshark and how did you use it?**
A: Found under Statistics → Protocol Hierarchy, it shows percentage breakdown of protocols in the capture. We used it to identify the dominant protocols in Hotstar traffic, revealing HTTP/HTTPS over TCP as primary protocols.

**Q: How do you use Wireshark's I/O Graph feature and what did it reveal about Hotstar?**
A: Found under Statistics → I/O Graph, it plots traffic volume over time. We used it to visualize throughput patterns, revealing spikes during initial buffering and quality changes, with consistent patterns during steady playback.

**Q: How did you examine TCP streams for video segment requests?**
A: By right-clicking a packet and selecting "Follow → TCP Stream", we could see the complete HTTP request/response exchange for video segments, including headers showing range requests and CDN information.

**Q: What is a display filter in Wireshark and which ones were most useful for this assignment?**
A: Display filters show only packets matching specific criteria. Most useful were: "http.request.uri contains .m3u8" for manifest files, "tcp.analysis.retransmission" for identifying network problems, and "ip.addr == [CDN IP]" for CDN-specific analysis.

**Q: How do you identify TLS handshakes in Wireshark?**
A: Using the display filter "tls.handshake.type" to find Client Hello, Server Hello, and Certificate messages, visible at the start of HTTPS connections to Hotstar's servers.

**Q: How did you measure Round Trip Time (RTT) in Wireshark?**
A: Using Statistics → TCP Stream Graphs → Round Trip Time Graph, which plots RTT values for selected TCP connections, allowing us to compare latency to different Hotstar servers.

**Q: What is the "Expert Information" feature in Wireshark and how did you use it?**
A: Found under Analyze → Expert Information, it highlights potential network issues like retransmissions and duplicate ACKs. We used it to identify problematic periods in Hotstar streaming sessions.

**Q: How did you analyze packet size distribution in Wireshark?**
A: Using Statistics → Packet Lengths, which groups packets by size ranges, showing that video segments primarily used 1400+ byte packets (near MTU size) while control messages used smaller packets.

**Q: How did you extract HTTP headers from encrypted HTTPS traffic?**
A: We couldn't directly view encrypted content, but analyzed TLS metadata, connection patterns, and used HTTP/2 headers visible during the unencrypted parts of TLS handshakes.

**Q: What Wireshark feature did you use to identify geographic locations of servers?**
A: We used Wireshark's ability to resolve IP addresses combined with external tools like whois and IP geolocation databases to map CDN server locations.

**Q: How did you use Wireshark's time reference feature?**
A: By right-clicking a packet and selecting "Set Time Reference", we established key points in the capture (like video start, quality changes, seeking) to measure time differences between streaming events.

**Q: How did you identify video segment requests in the packet list?**
A: By looking for HTTP GET requests with distinctive patterns in the URI (containing .ts, .m4s, or .mp4 extensions) and Range headers requesting specific byte ranges of media files.

**Q: What coloring rules were helpful in analyzing Hotstar traffic?**
A: We used custom coloring rules to highlight video segment requests in one color, manifest files in another, and TCP retransmissions in a warning color for quick visual identification in large captures. 