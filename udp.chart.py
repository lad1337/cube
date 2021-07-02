import sys
import time
import requests
import socket
from bases.FrameworkServices.SimpleService import SimpleService

priority = 90001

ORDER = [
    "udppusher",
]

CHARTS = {
    "udppusher": {
        "options": [None, "UDP-pusher", "percent", "cube", "metrics", "line"],
        "lines": [],
    }
}

PARAMS = {
    "format": "json",
    "chart": None,
    "dimension": None,
}


def history_key(metric):
    return f"{metric['chart']}.{metric['dimension']}"


class Service(SimpleService):
    def __init__(self, configuration=None, name=None):
        SimpleService.__init__(self, configuration=configuration, name=name)
        self.order = ORDER
        self.definitions = CHARTS
        self.url = self.configuration.get("url")
        self.target = self.configuration.get("target", {"url": None, "port": None})
        self.metrics = self.configuration.get("metrics", [])
        self._debug = self.configuration.get("debug", False)
        self._history = {
            m["dimension"]: {"min": m.get("min"), "max": m.get("max")} for m in self.metrics
        }
        self._got_history = False

    def check(self):
        if not self._got_history:
            time.sleep(5)
        check = False
        try:
            r = requests.get(self.url + "/charts").json()
            check = {m["chart"] for m in self.metrics} <= r["charts"].keys()
        except Exception as e:
            if self._debug:
                print(f"Check failed with error: {e}", file=sys.stderr)
            return False
        if check and not self._got_history:
            netdata = self._get_data(points=None, add_new=False, single=False, after=None)
            for k, values in netdata.items():
                for v in values:
                    if v:
                        self.normalize(k, v)
            self._got_history = True

        return check

    def normalize(self, dimension, value):
        history = self._history[dimension]
        if history["min"] is None:
            history["min"] = value
        if history["max"] is None:
            history["max"] = value

        history["min"] = min(value, history["min"])
        history["max"] = max(value, history["max"])
        if history["min"] == history["max"]:
            normalized = 0
        else:
            normalized = (value - history["min"]) / (history["max"] - history["min"])
        if self._debug:
            print("normal:", dimension, value, normalized, history, file=sys.stderr)
        return normalized

    def _get_data(self, points=3, add_new=True, single=True, after=-10):
        netdata = {}
        for metric in self.metrics:
            dimension = metric["dimension"]
            if add_new and dimension not in self.charts["udppusher"]:
                self.charts["udppusher"].add_dimension([dimension])
            PARAMS.update(
                {"chart": metric["chart"], "dimension": dimension, "points": points, "after": after}
            )
            r = requests.get(self.url + "/data", params=PARAMS)
            if r.status_code != 200:
                raise ValueError(f"Could not get data: {r.text}")
            r = r.json()
            if self._debug:
                print(r, file=sys.stderr)
            invert_factor = -1 if metric.get("invert", False) else 1
            if len(r["data"]):
                if single:
                    value = (r["data"][0][-1] or 0) * invert_factor
                else:
                    value = [(v[-1] or 0) * invert_factor for v in r["data"]]
            else:
                value = 0 if single else [0]
            netdata[dimension] = value
        return netdata

    def get_data(self):
        netdata = self._get_data()
        udp_data = []
        for key, value in netdata.items():
            value = self.normalize(key, value)
            netdata[key] = value * 100
            udp_data.append(str(round(value, 3)))

        socket.socket(socket.AF_INET, socket.SOCK_DGRAM).sendto(
            (",".join(udp_data)).encode("ascii"), (self.target["url"], self.target["port"])
        )
        return netdata
