"""Source-level regression guards; complementary to compiled Qt tests, not a substitute."""
from pathlib import Path
import re
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
TOKEN = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]')


def body(filename, signature):
    source = (ROOT / filename).read_text(encoding="utf-8")
    begin = source.index("{", source.index(signature))
    depth = 0
    for match in TOKEN.finditer(source, begin):
        if match.group() == "{":
            depth += 1
        elif match.group() == "}":
            depth -= 1
            if depth == 0:
                return source[begin:match.end()]
    raise AssertionError(f"Unterminated function: {signature}")


class SourceSafetyTests(unittest.TestCase):
    def test_frp_entry_is_blocked_before_any_generation_or_process_start(self):
        text = body("backendclass.cpp", "void BackEndClass::rx_StartRepairing(")
        gate = text.index("tool == Tool_SPD_FRP_FastBoot")
        self.assertLess(gate, text.index("getServerIMEInumber_updateGlobal"))
        self.assertLess(gate, text.index("adbProcess->start"))
        self.assertIn("getServerFastBoot_File();\n        return;", text)

    def test_frp_server_handler_is_a_terminal_no_network_no_charge_stub(self):
        text = body("backendclass.cpp", "bool BackEndClass::getServerFastBoot_File()")
        for forbidden in ("QNetwork", "QFile", "sendOperationDetails", "setArguments", "->start("):
            self.assertNotIn(forbidden, text)
        self.assertIn("return false;", text)
        self.assertIn("signal.isProcessCompleted = true;", text)
        self.assertIn("processStartStop = false;", text)

    def test_frp_shell_payloads_are_removed(self):
        text = (ROOT / "backendclass.cpp").read_text(encoding="utf-8")
        for forbidden in ("http://", "fastboot erase frp", "oem passwd", "signature_", "fastBoot_FileName+"):
            self.assertNotIn(forbidden, text)

    def test_frp_button_cannot_be_reenabled(self):
        text = (ROOT / "mainwindow.cpp").read_text(encoding="utf-8")
        self.assertNotIn("pb_FRP_SPD->setEnabled(true)", text)
        self.assertNotIn("tx_StartRepairing", body("mainwindow.cpp", "void MainWindow::on_pb_FRP_SPD_clicked()"))

    def test_preferences_do_not_save_or_restore_passwords(self):
        save = body("mainwindow.cpp", "void MainWindow::saveNewSettings()")
        load = body("mainwindow.cpp", "void MainWindow::seeSettingsFile()")
        self.assertNotIn("txt_Password", save)
        self.assertNotIn("setText", load.split("ui->txt_Password", 1)[-1].split(";", 1)[0])
        self.assertIn("loadLoginPreferences", load)
        self.assertIn("saveLoginPreferences", save)
        self.assertNotIn("processSettingsFileLine", (ROOT / "mainwindow.h").read_text())

    def test_no_embedded_login_credentials_in_ui(self):
        tree = ET.parse(ROOT / "mainwindow.ui")
        for name in ("txt_Password", "txt_UsernameEmail"):
            value = tree.findtext(f".//widget[@name='{name}']/property[@name='text']/string", default="")
            self.assertFalse(bool(value), "Embedded credential defaults must be empty")

    def test_debug_output_disabled_in_all_application_builds(self):
        self.assertRegex((ROOT / "MotoTool_MTK.pro").read_text(), r"(?m)^DEFINES \+= QT_NO_DEBUG_OUTPUT$")

    def test_remote_log_upload_removed(self):
        text = body("class_logfile.cpp", "void Class_LogFile::writeToLog(")
        for forbidden in ("QNetwork", "authorizedToken", "api_LogQString", "nam.post"):
            self.assertNotIn(forbidden, text)

    def test_charge_boundary_is_bounded_and_fail_closed(self):
        text = body("backendclass.cpp", "ADB_Commands BackEndClass::sendOperationDetailsToServerToReduceCredit()")
        self.assertIn("deadline.start(15000)", text)
        self.assertIn("reply->abort()", text)
        self.assertIn("status < 200 || status >= 300", text)
        self.assertIn("Security::parseChargeReply", text)
        self.assertTrue(text.rstrip().endswith("return ADB_SerialNumber;\n}"))

    def test_serial_bounds_precede_legacy_indexing(self):
        text = body("mainwindow.cpp", "bool MainWindow::checkAllOk4MetaMode(")
        self.assertLess(text.index("device < 0"), text.index("combos.at(device)"))
        self.assertLess(text.index("selected >= localSerialList.size()"), text.index("switch (device)"))
        self.assertIn("currentData().toString() != localSerialList.at(selected).portName()", text)


if __name__ == "__main__":
    unittest.main()
