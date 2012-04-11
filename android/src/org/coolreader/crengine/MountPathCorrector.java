package org.coolreader.crengine;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;

public class MountPathCorrector {
	private final File[] mountedRoots;
	private LinkCollection rootFileLinks = new LinkCollection(); 
	public MountPathCorrector(File[] mountedRoots) {
		this.mountedRoots = mountedRoots;
	}

	private static boolean pathStartsWith(String path, String pattern) {
		if (!path.startsWith(pattern))
			return false;
		if (path.length() == pattern.length())
			return true;
		if (pattern.charAt(pattern.length() - 1) == '/') {
			if (path.charAt(pattern.length() - 1) == '/')
				return true;
			return false;
		}
		if (path.charAt(pattern.length()) == '/')
			return true;
		return false;
	}
	private static class LinkInfo {
		public String path;
		public String linksTo;
		public LinkInfo(String path, String linksTo) {
			this.path = path;
			this.linksTo = linksTo;
		}
		@Override
		public String toString() {
			return "Link[" + path + " => " + linksTo + "]";
		}
		private static String concatPaths(String base, String tail) {
			if (base.charAt(base.length() - 1) == '/') {
				// base has /
				if (tail.charAt(0) == '/')
					return base + tail.substring(1);
				return base + tail;
			}
			if (tail.charAt(0) == '/')
				return base + tail;
			return base + '/' + tail;
		}
		/**
		 * Try to correct path base by substitution of links, to start with good path.
		 * @param goodBase is desired base path
		 * @param pathName is path to correct
		 * @return corrected string if match is found, null if no correction found
		 */
		public String correct(String goodBase, String pathName) {
			if (pathStartsWith(goodBase, path) && pathStartsWith(pathName, linksTo)) {
				String res = concatPaths(path, pathName.substring(linksTo.length()));
				if (pathStartsWith(res, goodBase))
					return res;
			} else if (pathStartsWith(goodBase, linksTo) && pathStartsWith(pathName, path)) {
				String res = concatPaths(linksTo, pathName.substring(path.length()));
				if (pathStartsWith(res, goodBase))
					return res;
			}
			return null;
		}
	}
	
	private static class LinkCollection {
		private ArrayList<LinkInfo> links;
		public LinkCollection() {
			links = new ArrayList<LinkInfo>(4);
		}
		public LinkCollection(LinkCollection v) {
			links = new ArrayList<LinkInfo>(v.links.size() + 2);
		}
		public void add(LinkInfo item) {
			links.add(item);
		}
		public void addLinksFromPath(File path) {
			for (;;) {
				String p = path.getAbsolutePath();
				String lnk = Engine.isLink(p);
				if (lnk != null)
					links.add(new LinkInfo(p, lnk));
				File base = path.getParentFile();
				if (base == null)
					return;
				path = base;
			}
		}
		@Override
		public String toString() {
			return "[" + links + "]";
		}
		/**
		 * Try to correct path base by substitution of links, to start with good path.
		 * @param goodBase is desired base path
		 * @param pathName is path to correct
		 * @return corrected string if match is found, null if no correction found
		 */
		public String correct(String goodBase, String pathName) {
			if (pathStartsWith(pathName, goodBase))
				return pathName; // no correction needed
			for (LinkInfo link : links) {
				String res = link.correct(goodBase, pathName);
				if (res != null)
					return res;
			}
			return null;
		}
	}

	public String normalizeIfPossible(String path) {
		String res = normalize(path);
		if (res == null)
			return path;
		return res;
	}

	/**
	 * Normalize given path by replacing of symlinks to use one of known roots. 
	 * @param path path to normalize.
	 * @return normalized path which starts from one of known roots, null if match is not found
	 */
	public String normalize(String path) {
		if (path == null)
			return null;
		File res = normalize(new File(path));
		if (res == null)
			return null;
		return res.getAbsolutePath();
	}

	/**
	 * Normalize given path by replacing of symlinks to use one of known roots. 
	 * @param path path to normalize.
	 * @return normalized path which starts from one of known roots, null if match is not found
	 */
	public File normalize(File path) {
		LinkCollection thisFileLinks = new LinkCollection();
		thisFileLinks.addLinksFromPath(path);
		if (path == null)
			return null;
		String pathName = path.getAbsolutePath();
		for (File root : mountedRoots) {
			String goodBase = root.getAbsolutePath();
			String s = rootFileLinks.correct(goodBase, pathName);
			if (s != null)
				return new File(s);
			s = thisFileLinks.correct(goodBase, pathName);
			if (s != null)
				return new File(s);
		}
		return null;
	}

	@Override
	public String toString() {
		return "MountPathCorrector [mountedRoots="
				+ Arrays.toString(mountedRoots) + ", rootFileLinks="
				+ rootFileLinks + "]";
	}
	
}
