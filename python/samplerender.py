#!/usr/bin/python3

import sys
import math

from PySide.QtCore import Signal, QPoint, QSize, Qt
from PySide.QtGui import (QApplication, QVBoxLayout, QMessageBox, QSlider, QSplitter, QWidget, QLabel)
from PySide.QtOpenGL import QGLWidget

try:
	from OpenGL import GL
except ImportError:
	app = QApplication(sys.argv)
	QMessageBox.critical(None, "OpenGL hellogl", "PyOpenGL must be installed to run this example.")
	sys.exit(1)

class Window(QWidget):
	def __init__(self):
		super(Window, self).__init__()

		self.glWidget = GLWidget()

		renderarea = QSplitter(Qt.Vertical)
		renderarea.addWidget(self.glWidget)

		self.dummy_object = QLabel("wat")

		mainlayout = QVBoxLayout()
		splitter = QSplitter()
		splitter.addWidget(renderarea)
		splitter.addWidget(self.dummy_object)
		mainlayout.addWidget(splitter)
		self.setLayout(mainlayout)

		self.setWindowTitle("Hello GL")

class GLWidget(QGLWidget):
	def __init__(self, parent=None):
		super(GLWidget, self).__init__(parent)

		self.xRot = 0
		self.yRot = 0
		self.lastPos = QPoint()

	def minimumSizeHint(self):
		return QSize(50, 50)

	def sizeHint(self):
		return QSize(400, 400)

	def setXRotation(self, angle):
		angle = self.normalizeAngle(angle)
		if angle != self.xRot:
			self.xRot = angle
			self.updateGL()

	def setYRotation(self, angle):
		angle = self.normalizeAngle(angle)
		if angle != self.yRot:
			self.yRot = angle
			self.updateGL()

	def initializeGL(self):
		# gl one-time init code
		pass

	def paintGL(self):
		# do ALL the rendering (clear etc)
		pass

	def resizeGL(self, width, height):
		side = min(width, height)
		if side < 0:
			return

		GL.glViewport((width - side) // 2, (height - side) // 2, side, side)
		# TODO make the changed perspective known to libdicraft-render

	def mousePressEvent(self, event):
		self.lastPos = event.pos()

	def mouseMoveEvent(self, event):
		dx = event.x() - self.lastPos.x()
		dy = event.y() - self.lastPos.y()

		if event.buttons() & Qt.LeftButton:
			self.setXRotation(self.xRot + 8 * dy)
			self.setYRotation(self.yRot + 8 * dx)

		self.lastPos = event.pos()

	def normalizeAngle(self, angle):
		while angle < 0:
			angle += 360 * 16
		while angle > 360 * 16:
			angle -= 360 * 16
		return angle

if __name__ == '__main__':
	app = QApplication(sys.argv)
	window = Window()
	window.show()
	sys.exit(app.exec_())
