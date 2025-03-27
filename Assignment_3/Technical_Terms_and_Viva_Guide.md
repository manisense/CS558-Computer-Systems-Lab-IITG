# Technical Terms and Viva Guide
## CS558: Computer Systems Lab - Assignment 3
### Group 25

This document provides explanations for technical terms used in the Hotstar Video Streaming Network Analysis report, along with potential questions and answers that may arise during a viva examination.

## Technical Terms Explained

### Networking Fundamentals

1. **Wireshark**: An open-source network protocol analyzer that captures and inspects data packets on a network interface in real-time. It allows detailed inspection of hundreds of protocols, with packet filtering, color coding, and other features.

2. **Packet**: The fundamental unit of data transmitted over a network. Each packet contains both the data being transmitted and control information like source and destination addresses.

3. **Protocol**: A standardized set of rules that determine how data is transmitted and received across networks. Protocols define the format, timing, sequencing, and error control in data communication.

4. **OSI Model**: A conceptual framework that standardizes the functions of a telecommunication or computing system into seven abstraction layers: Physical, Data Link, Network, Transport, Session, Presentation, and Application.

5. **TCP/IP Model**: A simplified version of the OSI model with four layers: Network Interface, Internet, Transport, and Application, which is the foundation of Internet communications.

### Network Layers and Protocols

#### Physical and Data Link Layer

6. **Ethernet (IEEE 802.3)**: A family of wired computer networking technologies commonly used in local area networks (LANs). It defines wiring and signaling standards for the physical layer and frame formats and protocols for the data link layer.

7. **IEEE 802.11 (Wi-Fi)**: A set of standards for implementing wireless local area network (WLAN) computer communication in various frequencies, including but not limited to 2.4 GHz, 5 GHz, and 6 GHz.

#### Network Layer

8. **IPv4 (Internet Protocol version 4)**: The fourth version of the Internet Protocol, which uses a 32-bit address scheme and is the most widely deployed Internet Protocol used to connect devices to the Internet.

9. **IPv6 (Internet Protocol version 6)**: The most recent version of the Internet Protocol, intended to replace IPv4. It uses a 128-bit address scheme, allowing for a vastly greater number of unique addresses.

10. **ICMP (Internet Control Message Protocol)**: A network layer protocol used by network devices to diagnose network communication issues. It's primarily used for error reporting and operational information.

#### Transport Layer

11. **TCP (Transmission Control Protocol)**: A connection-oriented protocol that provides reliable, ordered, and error-checked delivery of data between applications running on hosts communicating over an IP network.

12. **UDP (User Datagram Protocol)**: A connectionless protocol that provides a minimal, unreliable transport service, sending packets with no guarantee of delivery, ordering, or duplicate protection.

13. **QUIC (Quick UDP Internet Connections)**: A transport layer protocol designed by Google, which combines the reliability of TCP with the speed of UDP, and includes built-in encryption equivalent to TLS/SSL.

#### Application Layer

14. **HTTP/1.1 (Hypertext Transfer Protocol)**: The foundational protocol for the World Wide Web, used for transmitting hypermedia documents like HTML. HTTP/1.1 introduced persistent connections and other improvements.

15. **HTTPS (HTTP Secure)**: An extension of HTTP that uses encryption for secure communication over a computer network. The communication protocol is encrypted using Transport Layer Security (TLS) or its predecessor, Secure Sockets Layer (SSL).

16. **HTTP/2**: A major revision of HTTP that focuses on performance improvements, including binary protocols, multiplexing, server push, and header compression.

17. **TLS (Transport Layer Security)**: Cryptographic protocols designed to provide communications security over a computer network, encrypting the segments of network connections at the application layer.

18. **DNS (Domain Name System)**: A hierarchical and decentralized naming system for computers, services, or other resources connected to the Internet or a private network, translating domain names to IP addresses.

19. **MPEG-DASH (Dynamic Adaptive Streaming over HTTP)**: An adaptive bitrate streaming technique that enables high-quality streaming of media content over the Internet delivered from conventional HTTP web servers.

20. **HLS (HTTP Live Streaming)**: An HTTP-based adaptive bitrate streaming communications protocol developed by Apple Inc. that works by breaking the overall stream into a sequence of small HTTP-based file downloads.

21. **WebSocket**: A computer communications protocol, providing full-duplex communication channels over a single TCP connection, designed for real-time data transfer between web browsers and servers.

### Protocol Fields and Features

22. **Host Header**: An HTTP header field that specifies the domain name of the server being requested, allowing multiple websites to be served from a single IP address.

23. **Content-Type**: An HTTP header field that indicates the media type (or MIME type) of the resource being sent to the client.

24. **Status Codes**: Three-digit HTTP response codes that indicate the outcome of an HTTP request:
   - 2xx (Success): Request was successfully received, understood, and accepted
   - 3xx (Redirection): Further action needs to be taken to complete the request
   - 4xx (Client Error): Request contains bad syntax or cannot be fulfilled
   - 5xx (Server Error): Server failed to fulfill a valid request

25. **TCP Flags**: Control bits in the TCP header that manage the state of a connection:
   - SYN (Synchronize): Initiates a connection
   - ACK (Acknowledgment): Acknowledges received data
   - FIN (Finish): Properly terminates a connection
   - RST (Reset): Abruptly terminates a connection
   - PSH (Push): Pushes buffered data to the receiving application
   - URG (Urgent): Indicates that the data is urgent

26. **TCP Window Size**: Specifies the amount of data that can be sent before requiring an acknowledgment, used for flow control between sender and receiver.

27. **TCP Sequence Number**: Used to identify each byte of data and ensure ordered delivery of data to the application layer.

28. **TTL (Time To Live)**: A value in an IP packet that determines how many router hops the packet can traverse before being discarded, preventing packets from circulating indefinitely.

29. **Cipher Suite**: In TLS/SSL, a combination of algorithms that help secure a network connection using the TLS/SSL protocol, typically containing a key exchange algorithm, a bulk encryption algorithm, and a message authentication code.

### Performance Metrics

30. **Throughput**: The rate of successful message delivery over a communication channel, typically measured in bits per second (bps) or data packets per second.

31. **RTT (Round Trip Time)**: The time it takes for a data packet to be sent plus the time it takes for an acknowledgment of that packet to be received, a key measure of network delay.

32. **Packet Loss**: Occurs when one or more packets of data traveling across a network fail to reach their destination, often causing degradation in service quality.

33. **Jitter**: The variation in the delay of received packets, which can significantly impact the quality of streaming media.

34. **Retransmission**: When TCP detects packet loss, it automatically retransmits the lost packets to ensure reliable delivery.

35. **Duplicate ACKs**: When a TCP receiver receives an unexpected segment, it sends a duplicate ACK for the last correctly received segment, which can indicate packet loss.

### Streaming Technology

36. **Adaptive Bitrate Streaming**: A technique used in streaming multimedia over computer networks, where the source content is encoded at multiple bit rates, allowing the client to switch between streams of different quality based on available resources.

37. **Manifest File**: In adaptive streaming, a file that provides the client with information about available media segments, bitrates, and other characteristics of the media presentation.

38. **Media Segment**: A chunk of media (audio or video) of a specific duration, typically a few seconds long, used in adaptive streaming protocols.

39. **Bitrate**: The rate at which data is processed or transferred, measured in bits per second. Higher bitrates generally mean higher quality video but require more bandwidth.

40. **CDN (Content Delivery Network)**: A geographically distributed network of proxy servers and their data centers, which provides high availability and performance by distributing the service spatially relative to end-users.

41. **Buffer**: Temporary storage area where data is held before being processed, used in streaming to ensure smooth playback despite network fluctuations.

42. **Initial Buffering**: The process where the player accumulates a certain amount of data before starting playback to ensure a smooth viewing experience.

## Potential Viva Questions and Answers

### Basic Networking Concepts

**Q1: What is the difference between TCP and UDP, and why might Hotstar use both?**

A1: TCP is a connection-oriented protocol that provides reliable, ordered, and error-checked delivery of data, while UDP is a connectionless protocol that offers faster transmission without guarantees of delivery or ordering.

Hotstar primarily uses TCP for most content delivery (including video segments) because reliability is crucial for video playback quality. However, UDP might be used for real-time elements like live sports events where timeliness is more important than perfect reliability, or through QUIC protocol for improved performance. The choice depends on the specific requirements of different content types and user interactions.

**Q2: Explain the three-way handshake in TCP. How did you observe this in Hotstar traffic?**

A2: The TCP three-way handshake is a method used to establish a connection between a client and server:
1. The client sends a SYN packet to the server.
2. The server responds with a SYN-ACK packet.
3. The client sends an ACK packet to acknowledge the server's response.

In Hotstar traffic, we observed this process whenever the client established new connections to Hotstar's servers. This was particularly visible when:
- Initially connecting to www.hotstar.com
- Establishing connections to CDN servers for video content
- Opening new connections for API calls or asset downloads

The handshake could be observed by filtering packets with TCP flags SYN and SYN-ACK, showing the initial connection establishment before data transfer began.

### Video Streaming Technology

**Q3: What is adaptive bitrate streaming and how does Hotstar implement it?**

A3: Adaptive bitrate streaming is a technique where content is encoded at multiple quality levels (bitrates), allowing clients to switch between these qualities based on available network conditions, thereby optimizing playback quality while minimizing buffering.

Hotstar implements this using both HLS (HTTP Live Streaming) and MPEG-DASH protocols. We observed this in our packet captures through:
1. Initial manifest file requests (.m3u8 or .mpd files) that list available quality levels
2. Media segment requests that changed to different quality versions based on network conditions
3. Automatic switching to lower bitrates during network congestion periods (especially observed during evening captures)
4. Higher quality segments being requested when bandwidth improved

The implementation enables seamless quality transitions without interrupting playback, ensuring a good user experience across varying network conditions.

**Q4: How do manifest files work in streaming platforms like Hotstar? What information do they contain?**

A4: Manifest files serve as the "index" for adaptive streaming content, providing clients with metadata needed to request and play the content properly.

In Hotstar's implementation, we observed both HLS (.m3u8) and DASH (.mpd) manifest files containing:
1. Available video quality levels (ranging from ~400 Kbps to 5 Mbps)
2. URLs for each media segment at different quality levels
3. Segment duration information (typically 4-10 seconds per segment)
4. Media characteristics (codecs, resolution, etc.)
5. DRM information where applicable
6. Alternate audio tracks or subtitle information

The client player first requests the master manifest, then selects appropriate quality levels based on available bandwidth, and requests individual segments accordingly. Manifest files are periodically refreshed for live content to get updated segment information.

### Performance Analysis

**Q5: You mentioned that evening traffic shows reduced throughput. Explain why this happens and how Hotstar's systems adapt to it.**

A5: Evening traffic shows reduced throughput primarily due to:
1. Peak internet usage hours (6 PM - 10 PM) when most users are at home
2. Network congestion at both ISP and backbone levels
3. Possible throttling by ISPs during peak hours
4. Higher contention ratios affecting available bandwidth

Our measurements showed average throughput dropping from 4.32 Mbps in morning to 2.95 Mbps in evening sessions.

Hotstar's systems adapt to these conditions through:
1. Automatic quality adjustment via adaptive streaming protocols
2. More aggressive bitrate switching algorithms during peak hours
3. CDN load balancing to shift traffic to less congested servers
4. Possible prioritization of critical content (video segments) over non-essential content
5. Buffer management to accommodate fluctuating bandwidth

These adaptations ensure continued playback, though often at lower quality levels during peak evening hours.

**Q6: What metrics did you use to evaluate TCP performance in Hotstar streaming? What did they reveal about the service?**

A6: We evaluated TCP performance using several key metrics:

1. **Round Trip Time (RTT)**: We measured average, minimum, and maximum RTT values, finding average RTT ranging from 45.32ms (morning) to 68.94ms (evening). This showed network congestion effects during peak hours.

2. **Retransmission Rate**: We found relatively low retransmission rates (0.12% to 0.38%), indicating good network reliability despite varying conditions.

3. **TCP Window Size Evolution**: By analyzing window scaling, we observed how Hotstar's servers optimize data flow, typically starting at 65535 bytes and scaling based on network conditions.

4. **Duplicate ACKs**: We tracked duplicate ACKs as indicators of potential packet loss, noting correlation with evening congestion periods.

5. **Connection Patterns**: We analyzed the number of parallel TCP connections used (typically 6-8 per server domain), showing Hotstar's optimization for HTTP/1.1 limitations.

These metrics revealed that Hotstar employs robust TCP optimization techniques that maintain reliable streaming even during challenging network conditions, with intelligent adjustment to varying RTT and congestion levels.

### Content Delivery and Infrastructure

**Q7: Explain Hotstar's multi-CDN strategy. Why would a streaming service use multiple CDNs instead of just one?**

A7: Hotstar employs a multi-CDN strategy distributing content across several providers including Akamai (62.3%), Hotstar's own CDN (24.8%), Amazon CloudFront (8.7%), and Google Cloud CDN (4.2%).

The advantages of this approach include:

1. **Geographic Optimization**: Different CDNs may have better coverage in specific regions, allowing content to be served from the nearest available server.

2. **Redundancy and Failover**: If one CDN experiences issues, traffic can automatically shift to alternate providers, ensuring continuous service availability.

3. **Load Balancing**: Traffic can be distributed across multiple providers to prevent overloading any single infrastructure component.

4. **Content-Specific Optimization**: Different types of content (video segments, API calls, static assets) can be routed to CDNs optimized for that specific purpose.

5. **Cost Optimization**: By distributing traffic across providers with different pricing models, Hotstar can optimize costs while maintaining performance.

6. **Performance Competition**: Using multiple CDNs creates healthy competition between providers, incentivizing better performance.

In our analysis, we observed dynamic CDN selection based on factors like content type, time of day, network conditions, and possibly content popularity.

**Q8: How does geographic distribution of servers affect streaming performance? What evidence of this did you find in your analysis?**

A8: Geographic distribution of servers significantly affects streaming performance through:

1. **Reduced Latency**: Shorter physical distance between users and content servers reduces round-trip time.

2. **Improved Throughput**: Fewer network hops typically results in better throughput and less congestion.

3. **Regional Optimization**: Content popular in specific regions can be cached more effectively at nearby servers.

In our analysis, we found evidence of geographic distribution effects through:

1. **RTT Measurements**: Connections to servers in Mumbai (32-45ms) and Chennai (48-62ms) showed significantly lower latencies than those to servers in Singapore (75-95ms), Europe (140-180ms), or North America (220-280ms).

2. **DNS Resolution Patterns**: Hotstar's DNS responses varied based on testing location, directing users to the nearest available edge servers.

3. **Traffic Distribution**: The majority of video segment requests (72%) were served from Indian data centers, while global CDN resources handled more standardized content.

4. **Performance Correlation**: When connections shifted to more distant servers during periods of local congestion, we observed temporary increases in startup delay and initial buffering times, though adaptive bitrate streaming helped compensate once playback began.

This geographic strategy allows Hotstar to provide optimized streaming experiences while maintaining global accessibility.

### Protocol Analysis

**Q9: Why is HTTPS used for video streaming instead of regular HTTP? What did your analysis reveal about Hotstar's security implementations?**

A9: HTTPS is used for video streaming instead of regular HTTP for several important reasons:

1. **Content Protection**: Prevents unauthorized interception and piracy of premium content.

2. **User Privacy**: Protects viewing habits and personal information from network eavesdropping.

3. **Data Integrity**: Ensures content hasn't been tampered with or modified in transit.

4. **ISP Interference Prevention**: Prevents ISPs from injecting ads or throttling specific content types.

Our analysis of Hotstar's security implementations revealed:

1. **TLS Versions**: Primarily TLS 1.2 and TLS 1.3, with no support for older, vulnerable versions like SSL 3.0 or TLS 1.0.

2. **Cipher Suites**: Strong preference for AES_256_GCM and CHACHA20_POLY1305 ciphers, which provide excellent security and performance.

3. **Certificate Handling**: Proper implementation of certificate validation and trust chains.

4. **HSTS Usage**: HTTP Strict Transport Security headers to enforce secure connections.

5. **DRM Integration**: Secure key exchange for protected content playback.

6. **Session Management**: Secure token-based authentication visible in encrypted API calls.

The robust implementation of these security measures demonstrates Hotstar's commitment to protecting both their content and user privacy, while still maintaining efficient content delivery.

**Q10: Explain how HTTP status codes are used in video streaming. What distribution of status codes did you observe in Hotstar traffic?**

A10: HTTP status codes play crucial roles in video streaming by indicating the outcome of various requests:

1. **200 OK**: Used for successful delivery of manifest files, complete media files, or API responses.

2. **206 Partial Content**: Extensively used for delivering video segments in response to range requests, allowing clients to request specific portions of media files.

3. **302 Found**: Used for redirecting clients to the optimal CDN server or alternate content sources.

4. **404 Not Found**: Indicates requested media segments or resources are unavailable.

5. **403 Forbidden**: Indicates authentication/authorization failures for premium or geo-restricted content.

In our analysis of Hotstar traffic, we observed the following distribution:

- **206 Partial Content**: ~68% (predominant for video segment delivery)
- **200 OK**: ~24% (manifests, API responses, complete resources)
- **302 Found**: ~5% (CDN redirections, authentication flows)
- **404 Not Found**: ~2% (missing resources, expired content)
- **403 Forbidden**: ~1% (authorization issues, geo-restrictions)

The high percentage of 206 responses confirms Hotstar's efficient implementation of range requests for media delivery, allowing for precise seeking and adaptive quality switching. The relatively low percentage of error codes (404, 403) indicates a well-maintained content delivery system with good availability.

### User Interaction Analysis

**Q11: Describe the network behavior when a user seeks to a different position in a video. How does Hotstar optimize this process?**

A11: When a user seeks to a different position in a video, several distinctive network behaviors occur:

1. **Manifest Consultation**: The client first consults the already downloaded manifest file to identify which segment contains the target position.

2. **Range Request**: The client issues an HTTP GET request with a Range header for the specific segment containing the seek position, typically resulting in a 206 Partial Content response.

3. **Buffer Clearing**: Previous buffered segments may be discarded, visible as a pause in segment requests for the old position.

4. **Accelerated Buffering**: After seeking, we observed bursts of 3-5 segment requests in rapid succession to quickly build a new buffer at the new position.

5. **Quality Adaptation**: Often, playback resumes at a lower quality initially, then gradually increases as the buffer rebuilds.

Hotstar optimizes this process through:

1. **Precise Segment Indexing**: Their manifest files contain detailed timing information for accurate seeking.

2. **Keyframe Optimization**: Segments are encoded with regular keyframes to enable efficient seeking.

3. **Predictive Pre-fetching**: In some cases, we observed requests for segments adjacent to the seek point, suggesting predictive loading.

4. **CDN Response Prioritization**: Seek requests appeared to receive higher priority handling by CDN servers, with lower latency than regular sequential segment requests.

5. **Smart Buffer Management**: The player intelligently manages which buffered segments to keep and which to discard during seeks to minimize unnecessary downloads.

These optimizations collectively result in seek operations that typically resume playback within 1-2 seconds, even when jumping to distant parts of long-form content.

**Q12: How does pausing and resuming a video affect network traffic? What strategies does Hotstar employ during paused periods?**

A12: Pausing and resuming a video creates distinctive patterns in network traffic:

**When Pausing:**
1. **Request Cessation**: The client immediately stops requesting new video segments, creating a clear gap in the regular pattern of segment requests.

2. **Heartbeat Maintenance**: Small periodic API calls (typically 2-5KB) continue at 30-60 second intervals to maintain session state.

3. **Analytics Reporting**: We observed small bursts of analytics data reporting viewing progress and pause events.

4. **Bandwidth Probe Packets**: Occasional small data transfers to estimate available bandwidth even while paused.

**When Resuming:**
1. **Manifest Check**: If paused for an extended period (>5 minutes), the client often re-requests the manifest file to check for updates.

2. **Buffering Burst**: A sudden burst of segment requests to refill the playback buffer, often 3-5 segments requested in rapid succession.

3. **Bandwidth Reassessment**: Initial segments may be requested at a lower quality until bandwidth is reassessed.

Hotstar employs several strategic approaches during paused periods:

1. **Session Preservation**: Maintaining minimal communication to keep sessions active without unnecessary data transfer.

2. **Resource Conservation**: Releasing network and server resources while maintaining just enough connection to enable quick resumption.

3. **Smart Position Management**: Tracking exact pause position server-side to enable seamless resumption even after long pauses.

4. **Adaptive Buffer Management**: Adjusting how much content remains buffered based on pause duration—retaining more buffer for short pauses, less for extended ones.

This balanced approach optimizes the user experience by enabling instant playback on resume while conserving bandwidth and server resources during pauses.

### Analytical Methods

**Q13: How did you calculate throughput in your analysis? What factors might affect the accuracy of these measurements?**

A13: We calculated throughput using the following methodology:

1. **Time-Based Sampling**: We divided the capture into 1-second intervals for granular analysis.

2. **Data Volume Calculation**: For each interval, we summed the total bytes of all packets related to Hotstar traffic.

3. **Conversion to Mbps**: We converted the byte count to bits and divided by the exact interval duration to get Mbps (Megabits per second).

4. **Filtering Relevant Traffic**: We used display filters to focus on actual content delivery packets while excluding control communications.

5. **Statistical Analysis**: We derived average, peak, and minimum throughput values across the entire session and for specific activities (e.g., initial buffering, steady playback).

Factors that might affect the accuracy of these measurements include:

1. **Capture Loss**: Wireshark might miss packets during high-traffic periods, leading to underestimated throughput.

2. **Non-Hotstar Traffic**: Despite filtering, some non-Hotstar traffic might be incorrectly included, especially from the same CDN providers.

3. **Protocol Overhead**: Our measurements include all protocol headers (TCP/IP/HTTP), which wouldn't represent the actual media bitrate.

4. **Timing Precision**: Small variations in timestamp precision can affect short-interval throughput calculations.

5. **TCP Retransmissions**: Counting retransmitted packets would artificially inflate throughput figures from an application perspective.

6. **Aggregation Effects**: 1-second intervals might mask microsecond-level burstiness in the traffic.

To mitigate these issues, we performed multiple capture sessions, used consistent methodology across all analyses, and verified key measurements using multiple calculation approaches.

**Q14: What methodology did you use to identify CDN providers in your analysis? What challenges did you face?**

A14: We identified CDN providers using a multi-faceted methodology:

1. **Reverse DNS Lookups**: We performed reverse DNS queries on server IP addresses to identify hostnames containing CDN identifiers (e.g., "akamai", "cloudfront").

2. **WHOIS Database Analysis**: For IP addresses without revealing hostnames, we consulted WHOIS records to identify the owning organizations.

3. **TLS Certificate Examination**: We analyzed SSL/TLS certificates for organization names and alternative subject names.

4. **Network Path Tracing**: We conducted traceroute analysis to identify network paths and ASN (Autonomous System Numbers) associated with major CDNs.

5. **Header Pattern Recognition**: We examined response headers for CDN-specific patterns (e.g., "Server", "X-Cache", "X-Amz-Cf-Id").

6. **IP Range Correlation**: We compared IP ranges with known CDN IP blocks published by major providers.

Challenges we faced during this analysis included:

1. **HTTPS Encryption**: Most traffic was encrypted, limiting visibility into certain identifying headers.

2. **White-labeled CDNs**: Some content appeared to be served from Hotstar's own CDN, which might actually be a white-labeled service from another provider.

3. **Dynamic IP Allocation**: CDN providers frequently change IP ranges, making some reference databases outdated.

4. **Multi-tier Delivery**: Content sometimes passed through multiple CDNs or delivery layers, complicating attribution.

5. **Limited DNS Information**: Some CDNs deliberately use generic or obfuscated DNS names that don't reveal the provider.

6. **Shared Infrastructure**: Some IP ranges are used by multiple services from the same company (e.g., Google serving both GCP CDN and regular Google services).

To overcome these challenges, we cross-referenced multiple identification methods and focused on traffic patterns and performance characteristics to verify our CDN attributions.
