# -Real-time-processing-system-simulator
Real-time processing system simulator receiving eight-bit data from N sources and sending packets of length M of data over K independent communication channels. Sources send data in the following mode: - a package containing no more than 100 independent data, - a stream of 30 to 500 related data.
Data in the packet are transferred every 0.5ms and are independent of each other. The data in the stream is transferred every 0.1ms and is related to each other and their order is important.
Receiving a single data and writing it to the communication buffer takes b * 1us and reading the data from the communication buffer takes b * 1us (b - buffer size). In total, communication buffers can hold BK of data. The time of sending a packet in one communication channel is M * 1us
Packages can have 4 forms:
- A: M package data
- B: P zero data and T stream data (P + T = M)
- C: T stream data and P packet data (T + P = M)
- D: M data from the stream
Assuming the possibility of data loss at the level of:
package - up to 10% on average
stream - up to 5% on average
how many more sources the simulated system can handle. Whether and if increasing the number of buffers by 1 (maintaining the total size of B) in this case reduces the level of data loss.
