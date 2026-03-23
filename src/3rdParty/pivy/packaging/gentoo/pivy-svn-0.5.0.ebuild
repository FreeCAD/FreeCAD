# Copyright 1999-2007 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

inherit eutils distutils subversion

ESVN_REPO_URI="http://svn.coin3d.org/repos/Pivy/trunk"

DESCRIPTION="Python Coin bindings"
HOMEPAGE="http://pivy.coin3d.org/"
LICENSE="as-is"
SLOT="0"
KEYWORDS="-* ~amd64 ~x86"
IUSE=""

DEPEND="virtual/python
	dev-lang/swig
	media-libs/coin
	media-libs/SoQt"
RDEPEND="${DEPEND}"
