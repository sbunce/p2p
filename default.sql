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
VALUES ('44EAAFB02EB1EF5B54D58C4582FBAC35A47341F6', 'Kings of Power 4 Billion %.avi', '337080320', 'topaz;emerald;ruby');
INSERT INTO search (hash, name, size, server)
VALUES ('93E1EFA2EBCD23112699B9FED27CE06F5EF3483C', 'Primer.avi', '734001152', 'topaz');
