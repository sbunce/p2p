DROP TABLE IF EXISTS search;
CREATE TABLE search (hash TEXT, name TEXT, size INTEGER, server_IP TEXT, file_ID TEXT);
CREATE UNIQUE INDEX IF NOT EXISTS nameIndex ON search (name);
INSERT INTO search (hash, name, size, server_IP, file_ID)
VALUES ('426CE948672CF42682F23AED2DF7C8F11CE693CD', 'Double Dragon.avi', '23842790', '192.168.1.3,192.168.1.4,192.168.1.5,192.168.1.114', '1,1,1,1');
INSERT INTO search (hash, name, size, server_IP, file_ID)
VALUES ('EF14E1F157601188D4C8B1DC905FC0664B5FBF86', 'Double Dragon 2.avi', '20576554', '192.168.1.3,192.168.1.4,192.168.1.5,192.168.1.114', '2,2,2,2');
INSERT INTO search (hash, name, size, server_IP, file_ID)
VALUES ('05EB36B483CCEAE80DB4852FDEC81981351974B2', 'Excitebike.avi', '21953008', '192.168.1.3,192.168.1.4,192.168.1.5,192.168.1.114', '3,3,3,3');
INSERT INTO search (hash, name, size, server_IP, file_ID)
VALUES ('669D5F2A93B4B2D77BF451ED7C2B658379BFAE4D', 'Super Mario 2 (Jap).avi', '21208890', '192.168.1.3,192.168.1.4,192.168.1.5,192.168.1.114', '4,4,4,4');
INSERT INTO search (hash, name, size, server_IP, file_ID)
VALUES ('4C26E2B10E371C3C7CAAC8D76FA02463EF4B6AB0', 'Primer.avi', '734001152', '192.168.1.3,192.168.1.4,192.168.1.5,192.168.1.114', '5,5,5,5');

DROP TABLE IF EXISTS share;
CREATE TABLE share (ID INTEGER PRIMARY KEY AUTOINCREMENT, hash TEXT, size INTEGER, path TEXT);
CREATE UNIQUE INDEX pathIndex ON share (path);
INSERT INTO share (ID, hash, size, path)
VALUES ('1', '426CE948672CF42682F23AED2DF7C8F11CE693CD', '23842790', '/home/smack/simple-p2p/share/Double Dragon.avi');
INSERT INTO share (ID, hash, size, path)
VALUES ('2', 'EF14E1F157601188D4C8B1DC905FC0664B5FBF86', '20576554', '/home/smack/simple-p2p/share/Double Dragon 2.avi');
INSERT INTO share (ID, hash, size, path)
VALUES ('3', '05EB36B483CCEAE80DB4852FDEC81981351974B2', '21953008', '/home/smack/simple-p2p/share/Excitebike.avi');
INSERT INTO share (ID, hash, size, path)
VALUES ('4', '669D5F2A93B4B2D77BF451ED7C2B658379BFAE4D', '21208890', '/home/smack/simple-p2p/share/Super Mario 2 (Jap).avi');
INSERT INTO share (ID, hash, size, path)
VALUES ('5', '4C26E2B10E371C3C7CAAC8D76FA02463EF4B6AB0', '734001152', '/home/smack/simple-p2p/share/Primer.avi');
