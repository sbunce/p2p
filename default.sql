DROP TABLE IF EXISTS search;
CREATE TABLE search (hash TEXT, name TEXT, size INTEGER, server TEXT);
CREATE INDEX IF NOT EXISTS name_index ON search (name);
INSERT INTO search (hash, name, size, server)
VALUES ('5B93E438BC9CCC1FF5861BECBD33D64C310F3802', 'Double Dragon.avi', '23842790', 'topaz;emerald;ruby');
INSERT INTO search (hash, name, size, server)
VALUES ('2EBB6151F51384A3925DF58BD2AF1906E22DB2EB', 'Double Dragon 2.avi', '20576554', 'topaz;emerald;ruby');
INSERT INTO search (hash, name, size, server)
VALUES ('C3B08A1FD1F0DAD8C1805274EE0E2297197B62BE', 'Excitebike.avi', '21953008', 'topaz;emerald;ruby');
INSERT INTO search (hash, name, size, server)
VALUES ('19E86CF99ABA74775A0C3CD5D298CCF9C104D86C', 'Super Mario 2 (Jap).avi', '21208890', 'topaz;emerald;ruby');
INSERT INTO search (hash, name, size, server)
VALUES ('8F54550390EBF50112BAF90F8A48BABFD1743B99', 'Kings of Power 4 Billion %.avi', '337080320', 'topaz;emerald;ruby');
INSERT INTO search (hash, name, size, server)
VALUES ('42341E64ED29BB4EB7D37EAB52F5E5A734448AAE', 'Primer.avi', '734001152', 'topaz');
