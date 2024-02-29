# MemoryManager

## High Level Architecture

### Blob Manager
 - All the insert/delete/lookup operations go through the Blob Manager.
 - It has a complete view of all the Node Managers and data-stores registered.
 - A blob manager can manage multiple Node managers. 
 - Operations
     - Insert:
         - Break the file into SEGMENT_SIZE size of chunks(segments).
         - Use consistent hahsing to determine the data-store for each segment.
         - A bidirectional stream RPC is established between the blob manager and each Node manager (single client, multiple servers) where the segments has to stored.
     - Lookup:
         - Lookup all the segment IDs for the file name to be looked up.
         - Use consitent hashing to determine the data-store to query for each segment.
         - Initiate a unary RPC to the Node manager where the segment is stored.
     - Delete:
        - Similar to lookup case, find all the segment IDs for the file to be deleted and send a unary RPC to the Node manager owning the segment to delete from the DB.

### Node Manager
 - Manage the actual task of writing/reading data into/from the DB.
 - A single node manager can manage multiple data-stores. 
 - The data base used as now is Mongo DB.
 - A Mongo client pool is used to handle multiple operaions concurrently.
