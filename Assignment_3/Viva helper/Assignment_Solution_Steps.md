# Assignment Solution Methodology
## CS558: Computer Systems Lab - Assignment 3
### Group 25

This document explains the step-by-step process we followed to address each major question in the Hotstar Video Streaming analysis assignment.

## 1. Identifying Protocols Used by Hotstar

### Steps Followed:
1. **Capture Setup**:
   - Applied capture filter: `host www.hotstar.com or host *.hotstar.com or host *.hotstarext.com`
   - Ensured clean test environment by closing other applications
   - Utilized Wi-Fi interface for capturing traffic

2. **Protocol Identification**:
   - Used Wireshark's Statistics → Protocol Hierarchy to obtain breakdown
   - Documented percentage contribution of each protocol
   - Identified top-level protocols and their hierarchy

3. **Layer-by-Layer Analysis**:
   - Examined physical/data link layer (Ethernet, IEEE 802.11)
   - Analyzed network layer (IPv4, IPv6, ICMP)
   - Investigated transport layer (TCP, UDP, QUIC)
   - Documented application layer (HTTP/HTTPS, TLS, DNS)

4. **Validation**:
   - Cross-verified protocol identification through packet inspection
   - Confirmed protocols by examining packet headers
   - Used display filters to isolate and study each protocol individually

## 2. Protocol Field Value Analysis

### Steps Followed:
1. **Field Identification**:
   - Created display filters for each protocol to examine
   - For IP: `ip`, for TCP: `tcp`, for HTTP: `http`, etc.
   - Enabled all columns related to fields of interest

2. **Data Collection**:
   - Captured multiple streaming sessions to ensure representative data
   - Created packet captures during different user interactions
   - Exported field values to spreadsheets for statistical analysis

3. **Statistical Analysis**:
   - Calculated average, minimum, maximum, and distribution of field values
   - Identified patterns in field usage (e.g., typical TCP window sizes)
   - Correlated field values with streaming events (buffering, quality changes)

4. **Field Value Documentation**:
   - Documented common values for source/destination ports
   - Recorded TTL values to estimate network distance
   - Analyzed TCP flags usage patterns
   - Documented HTTP headers including content types and status codes

## 3. Message Exchange Sequence Analysis

### Steps Followed:
1. **Connection Establishment Analysis**:
   - Filtered for TCP handshakes: `tcp.flags.syn==1`
   - Tracked complete connection setup sequences
   - Documented TLS handshakes using `tls.handshake.type`

2. **Initial Content Loading**:
   - Created time sequence diagram for initial page load
   - Identified DNS queries, TCP connections, and HTTP requests
   - Tracked dependencies between resources

3. **Video Playback Analysis**:
   - Captured Hotstar playback startup
   - Identified manifest file requests using `http.request.uri contains ".m3u8" or http.request.uri contains ".mpd"`
   - Tracked segment downloading patterns using `http.request.uri contains ".ts" or http.request.uri contains ".m4s"`

4. **User Interaction Sequences**:
   - Performed and captured specific actions:
     - Pausing/resuming video
     - Seeking to different positions
     - Changing quality settings
   - Documented network behavior for each interaction

5. **Flow Graph Creation**:
   - Used Statistics → Flow Graph to visualize message sequences
   - Highlighted key exchanges in the playback process
   - Identified patterns in request/response timing

## 4. Protocol Relevance Analysis

### Steps Followed:
1. **Protocol Function Identification**:
   - Examined each protocol's role in the streaming process
   - Used packet inspection to identify protocol dependencies
   - Documented protocol layering and encapsulation

2. **Performance Impact Assessment**:
   - Evaluated how each protocol contributes to streaming performance
   - Analyzed protocol overhead (headers, handshakes)
   - Identified optimization features used by each protocol

3. **Adaptive Streaming Analysis**:
   - Examined how HLS/DASH protocols implement adaptive bitrate
   - Analyzed manifest files structure and segment requests
   - Documented quality switching mechanisms

4. **Core Function Mapping**:
   - Mapped protocols to key streaming functions:
     - Content discovery
     - Video playback
     - Live streaming
     - User experience features
   - Documented how protocols work together to deliver content

## 5. Network Statistics Collection

### Steps Followed:
1. **Throughput Measurement**:
   - Used Statistics → I/O Graph with bits/second Y-Axis
   - Created 1-second interval measurements
   - Exported data for multiple streaming sessions
   - Calculated averages for different times of day

2. **RTT Analysis**:
   - Used Statistics → TCP Stream Graphs → Round Trip Time
   - Selected connections to Hotstar servers
   - Documented RTT variation over time
   - Calculated statistical measures (avg, min, max, stddev)

3. **Packet Size Distribution**:
   - Used Statistics → Packet Lengths
   - Created custom ranges for analysis (0-100, 101-500, etc.)
   - Documented distribution percentages
   - Correlated packet sizes with content types

4. **Protocol Distribution**:
   - Extracted protocol percentages from Protocol Hierarchy
   - Calculated both packet count and byte count percentages
   - Created comparative visualizations

5. **Packet Loss Analysis**:
   - Filtered for retransmissions: `tcp.analysis.retransmission`
   - Counted duplicate ACKs: `tcp.analysis.duplicate_ack`
   - Calculated loss rates for different capture periods

6. **Request-Response Analysis**:
   - Created filters for different request types
   - Measured response times for each request type
   - Calculated request-response ratios

## 6. Content Source Analysis

### Steps Followed:
1. **CDN Identification**:
   - Performed reverse DNS lookups on server IPs
   - Used external WHOIS database queries
   - Analyzed TLS certificate information
   - Examined HTTP response headers for CDN signatures

2. **Traffic Distribution Measurement**:
   - Filtered traffic by server IP
   - Calculated percentage of traffic per CDN
   - Documented which content types came from which CDNs

3. **Geographic Analysis**:
   - Used IP geolocation databases to map server locations
   - Correlated server locations with RTT measurements
   - Created geographical distribution map of content sources

4. **CDN Strategy Assessment**:
   - Analyzed patterns in CDN usage based on:
     - Time of day
     - Content type
     - Network conditions
     - Geographic location
   - Documented load balancing and redundancy mechanisms

5. **Dynamic Source Selection**:
   - Monitored changes in content sources during streaming sessions
   - Identified triggers for CDN switching
   - Documented failover behaviors

## 7. Data Visualization and Reporting

### Steps Followed:
1. **Chart Generation**:
   - Exported raw data to spreadsheets
   - Created visualizations for:
     - Protocol distribution (pie charts)
     - Throughput over time (line graphs)
     - RTT variation (line graphs)
     - Packet size distribution (bar charts)
     - CDN distribution (pie charts)

2. **Capture Organization**:
   - Organized captures by time of day and content type
   - Created consistent naming conventions
   - Documented metadata for each capture

3. **Screenshot Collection**:
   - Captured Wireshark interface showing key findings
   - Documented protocol hierarchies
   - Saved visualizations of performance metrics
   - Preserved examples of important packet exchanges

4. **Report Compilation**:
   - Organized findings into structured report
   - Created tables for statistical data
   - Included referenced screenshots
   - Documented methodologies for reproducibility
   - Developed conclusions based on comprehensive analysis

## Verification and Validation

Throughout the analysis process, we employed several validation techniques:

1. **Multiple Capture Sessions**:
   - Repeated measurements across different days and times
   - Compared results for consistency

2. **Cross-Verification**:
   - Used multiple methods to verify the same metric
   - Compared Wireshark's built-in statistics with manual calculations

3. **Control Captures**:
   - Created baseline captures with known traffic patterns
   - Used these to verify measurement accuracy

4. **Peer Review**:
   - Team members independently analyzed the same captures
   - Compared findings to identify discrepancies
   - Resolved differences through additional investigation

This methodical approach ensured thorough analysis and reliable findings for all aspects of Hotstar's video streaming implementation. 