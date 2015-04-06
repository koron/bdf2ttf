using System;
using System.Drawing;
using System.Drawing.Text;

public class FontList
{
    static public void Main()
    {
	InstalledFontCollection correction = new InstalledFontCollection();
	FontFamily[] family = correction.Families;
	for (int i = 0; i < family.Length; ++i)
	{
	    Console.WriteLine("[{0}] {1}", i, family[i].Name);
	}
    }
}
