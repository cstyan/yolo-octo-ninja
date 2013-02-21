all:
	$(MAKE) -C src

# Force make console version.
console:
	$(MAKE) F="../server.exe ../client.exe" -C src

# Force make windowed version, no console is created
windows:
	$(MAKE) F="../server.exe ../client.exe" W=-mwindows -C src

strip:
	strip client.exe server.exe

clean:
	$(MAKE) clean -C src