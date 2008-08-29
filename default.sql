DROP TABLE IF EXISTS search;
CREATE TABLE search (hash TEXT, name TEXT, size INTEGER, server TEXT);
CREATE INDEX IF NOT EXISTS name_index ON search (name);
INSERT INTO search (hash, name, size, server)
VALUES ('6E0F7F9554A1DD9D5281776FD98B7E3E1B9F3098', 'Double Dragon.avi', '23842790', 'topaz;emerald;ruby');
INSERT INTO search (hash, name, size, server)
VALUES ('56F99BBDBFFB68C0FD63F665759F96B0BED2EE0E', 'Double Dragon 2.avi', '20576554', 'topaz;emerald;ruby');
INSERT INTO search (hash, name, size, server)
VALUES ('2DC4BEC5BC8A572C9CD315FDC66C4C34C68F7F3E', 'Excitebike.avi', '21953008', 'topaz;emerald;ruby');
INSERT INTO search (hash, name, size, server)
VALUES ('A572A554B101636A3A6308CCF7D3AD88D84E2247', 'Super Mario 2 (Jap).avi', '21208890', 'topaz;emerald;ruby');
INSERT INTO search (hash, name, size, server)
VALUES ('EBF74EA0E6F980E1F5072084EA16F167CB048308', 'Kings of Power 4 Billion %.avi', '337080320', 'topaz;emerald;ruby');
INSERT INTO search (hash, name, size, server)
VALUES ('75EC2B493DF470BDCA5592BF0FD45ECC20ED35A1', 'Primer.avi', '734001152', 'topaz');
