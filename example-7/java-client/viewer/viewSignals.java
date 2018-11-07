
package viewer;

public interface viewSignals {
	public	void	tableSelect_withLeft	(String s);
	public	void	gainValue		(int gainValue);
	public	void	reset			();
	public	void	autogainButton		();
	public	void	channelSelect		(String s);
	public	void	lnaStateValue		(int lnaState);
	public	void	soundLevel		(int soundLevel);
}

