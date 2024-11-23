import asyncio
import logging
import os
import random
import string
import unittest
from functools import wraps

import omni.client
import omni.usd_resolver
from pxr import Ar, Sdf, Usd

TEST_USER = os.environ.get("OMNI_TEST_USER", "omniverse")
TEST_PASS = os.environ.get("OMNI_TEST_PASS", "omniverse")
TEST_HOST = os.environ.get("OMNI_TEST_HOST", "localhost")
BASE_URL = "omniverse://" + TEST_HOST + "/Tests/" + TEST_USER

RANDOM_URL = BASE_URL + "/test_python/" + "".join(random.choices(string.ascii_uppercase + string.digits, k=8))

TESTSTAGE_URL = f"{RANDOM_URL}/TestStage"
TESTSTAGE_EXPECTED = set(["cube", "models", "Root.usda"])

# Temporary disable all tests that connect to omni, to run them only locally
DISABLE_ALL_ONLINE_TESTS = False

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
BUILD_DIR = os.path.join(SCRIPT_DIR, "..", "..", "..", "..", "..")
TEST_DIR = os.path.join(BUILD_DIR, "test")


def log_handler(thread, component, level, message):
    # log to the python logger from omni.client
    # results will be collected by repo_test using the XmlTestRunner or TextTestRunner
    logger = logging.getLogger(__name__)
    if level == omni.client.LogLevel.DEBUG or level == omni.client.LogLevel.VERBOSE:
        logger.debug(message)
    elif level == omni.client.LogLevel.INFO:
        logger.info(message)
    elif level == omni.client.LogLevel.WARNING:
        logger.warning(message)
    elif level == omni.client.LogLevel.ERROR:
        logger.error(message)


def asyncio_wrap(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        asyncio.get_event_loop().run_until_complete(func(*args, **kwargs))

    return wrapper


class TestClient(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        logger = logging.getLogger(__name__)
        logger.setLevel(logging.DEBUG)

        handler = logging.FileHandler(os.path.abspath(os.path.join(TEST_DIR, f"{__name__}.log")))
        handler.setLevel(logging.DEBUG)

        logger.addHandler(handler)

        omni.client.initialize()
        omni.client.set_log_level(omni.client.LogLevel.DEBUG)
        omni.client.set_log_callback(log_handler)

        with omni.client.register_authorize_callback(default_authorize_callback):
            print(f"Coping TestStage to {TESTSTAGE_URL}")
            omni.client.copy("test-data/TestStage", TESTSTAGE_URL)

    @classmethod
    def tearDownClass(cls):
        omni.client.delete(RANDOM_URL)
        omni.client.shutdown()

    @unittest.skipIf(DISABLE_ALL_ONLINE_TESTS, "")
    @asyncio_wrap
    async def test_create_invalid_server(self):
        with omni.client.register_connection_status_callback(lambda url, status: None):
            previous_values = omni.client.set_retries(0, 0, 0)
            try:
                stage = Usd.Stage.CreateNew("omniverse://invalid_server/fake/path/a.usd")
            except:
                pass
            omni.client.set_retries(*previous_values)

    @unittest.skipIf(DISABLE_ALL_ONLINE_TESTS, "")
    @asyncio_wrap
    async def test_checkpoint_usd(self):
        omni.client.reconnect(BASE_URL)
        (server_info_result, server_info) = await omni.client.get_server_info_async(BASE_URL)
        self.assertEqual(server_info_result, omni.client.Result.OK)
        if not server_info.checkpoints_enabled:
            self.skipTest("Server doesn't support checkpoints")
            return

        TEST_CHECKPOINT_BASE_URL = f"{RANDOM_URL}/test_checkpoint_usd"
        TEST_CHECKPOINT_STAGE_URL = f"{TEST_CHECKPOINT_BASE_URL}/Root.usda"
        TEST_CHECKPOINT_REF_RELATIVE = "./cube/TwoCubes_set.usda"
        TEST_CHECKPOINT_REF_URL = omni.client.normalize_url(
            f"{TEST_CHECKPOINT_BASE_URL}/{TEST_CHECKPOINT_REF_RELATIVE}"
        )
        TEST_CHECKPOINT_PRIM = "/set/Cubes_grp/Cube_1/Geom/Cube"

        # Copy the test stage to a new folder so we can modify it
        result = await omni.client.copy_async(TESTSTAGE_URL, TEST_CHECKPOINT_BASE_URL)
        self.assertEqual(result, omni.client.Result.OK)

        # Create a checkpoint of the referenced layer
        (result, query) = await omni.client.create_checkpoint_async(TEST_CHECKPOINT_REF_URL, "Checkpoint")
        self.assertIn(result, [omni.client.Result.OK, omni.client.Result.ERROR_ALREADY_EXISTS])

        events = []

        def event_callback(url, event, state, file_size):
            nonlocal events
            events += [(url, event, state, file_size)]

        with omni.usd_resolver.register_event_callback(event_callback):
            # Update the root layer to reference the checkpoint
            layer = Sdf.Layer.FindOrOpen(TEST_CHECKPOINT_STAGE_URL)
            self.assertTrue(layer)

        self.assertGreaterEqual(len(events), 4)
        self.assertEqual(
            events[0],
            (TEST_CHECKPOINT_STAGE_URL, omni.usd_resolver.Event.RESOLVING, omni.usd_resolver.EventState.STARTED, 0),
        )
        self.assertEqual(events[1][0], TEST_CHECKPOINT_STAGE_URL)
        self.assertEqual(events[1][1], omni.usd_resolver.Event.RESOLVING)
        self.assertEqual(events[1][2], omni.usd_resolver.EventState.SUCCESS)
        self.assertGreater(events[1][3], 0)  # Not checking actual size in case the asset changes.

        if len(events) >= 6 and events[2][1] == omni.usd_resolver.Event.RESOLVING:
            self.assertEqual(
                events[4],
                (TEST_CHECKPOINT_STAGE_URL, omni.usd_resolver.Event.READING, omni.usd_resolver.EventState.STARTED, 0),
            )
            self.assertEqual(events[5][0], TEST_CHECKPOINT_STAGE_URL)
            self.assertEqual(events[5][1], omni.usd_resolver.Event.READING)
            self.assertEqual(events[5][2], omni.usd_resolver.EventState.SUCCESS)
            self.assertGreater(events[5][3], 0)  # Not checking actual size in case the asset changes.
        else:
            self.assertEqual(
                events[2],
                (TEST_CHECKPOINT_STAGE_URL, omni.usd_resolver.Event.READING, omni.usd_resolver.EventState.STARTED, 0),
            )
            self.assertEqual(events[3][0], TEST_CHECKPOINT_STAGE_URL)
            self.assertEqual(events[3][1], omni.usd_resolver.Event.READING)
            self.assertEqual(events[3][2], omni.usd_resolver.EventState.SUCCESS)
            self.assertGreater(events[3][3], 0)  # Not checking actual size in case the asset changes.

        update = layer.UpdateExternalReference(TEST_CHECKPOINT_REF_RELATIVE, TEST_CHECKPOINT_REF_RELATIVE + "?&1")
        self.assertTrue(update)

        layer.Save()

        # Create a checkpoint of the root layer
        (result, query) = await omni.client.create_checkpoint_async(TEST_CHECKPOINT_STAGE_URL, "Checkpoint")
        self.assertIn(result, [omni.client.Result.OK, omni.client.Result.ERROR_ALREADY_EXISTS])

        # Load the root layer from the checkpoint
        stage = Usd.Stage.Open(TEST_CHECKPOINT_STAGE_URL + "?&1")
        self.assertTrue(stage)

        # Make sure the referenced prim exists
        prim = stage.GetPrimAtPath(TEST_CHECKPOINT_PRIM)
        self.assertTrue(prim)

    @unittest.skipIf(DISABLE_ALL_ONLINE_TESTS, "")
    @asyncio_wrap
    async def test_checkpoint_message(self):
        omni.client.reconnect(BASE_URL)

        (server_info_result, server_info) = await omni.client.get_server_info_async(BASE_URL)
        self.assertEqual(server_info_result, omni.client.Result.OK)
        if not server_info.checkpoints_enabled:
            self.skipTest("Server doesn't support checkpoints")
            return

        checkpoint_message1 = "test checkpoint message 1"
        omni.usd_resolver.set_checkpoint_message(checkpoint_message1)

        LAYER_URL = f"{RANDOM_URL}/test_checkpoint_message.usd"
        layer = Sdf.Layer.CreateNew(LAYER_URL)

        checkpoint_message2 = "test checkpoint message 2"
        omni.usd_resolver.set_checkpoint_message(checkpoint_message2)

        layer.Save(False)

        (result, entries) = await omni.client.list_checkpoints_async(LAYER_URL)
        self.assertEqual(result, omni.client.Result.OK)
        self.assertEqual(len(entries), 2)
        self.assertIn(checkpoint_message1, entries[0].comment)
        self.assertIn(checkpoint_message2, entries[1].comment)

    @unittest.skipIf(DISABLE_ALL_ONLINE_TESTS, "")
    @asyncio_wrap
    async def test_acls_after_save(self):
        omni.client.reconnect(BASE_URL)

        LAYER_URL = f"{RANDOM_URL}/test_acls.usd"
        layer = Sdf.Layer.CreateNew(LAYER_URL)

        (result, acls) = await omni.client.get_acls_async(LAYER_URL)
        self.assertEqual(result, omni.client.Result.OK)
        self.assertGreater(len(acls), 0)

        for acl in acls:
            if acl.name == "users":
                acl.access = omni.client.AccessFlags.READ
                break
        else:
            acls.append(omni.client.AclEntry("users", omni.client.AccessFlags.READ))

        (result) = await omni.client.set_acls_async(LAYER_URL, acls)
        self.assertEqual(result, omni.client.Result.OK)

        (result, acls) = await omni.client.get_acls_async(LAYER_URL)
        self.assertEqual(result, omni.client.Result.OK)
        self.assertGreater(len(acls), 0)

        for acl in acls:
            if acl.name == "users":
                self.assertEqual(acl.access, omni.client.AccessFlags.READ)
                break
        else:
            self.fail("Did not find 'users' in acl list before saving")

        layer.Save(True)

        (result, acls) = await omni.client.get_acls_async(LAYER_URL)
        self.assertEqual(result, omni.client.Result.OK)
        self.assertGreater(len(acls), 0)

        for acl in acls:
            if acl.name == "users":
                self.assertEqual(acl.access, omni.client.AccessFlags.READ)
                break
        else:
            self.fail("Did not find 'users' in acl list after saving")

    @unittest.skipIf(DISABLE_ALL_ONLINE_TESTS, "")
    @asyncio_wrap
    async def test_resolve_no_extension(self):
        layerIdentifier = "test-data/TestStage/"

        # this will test OM-60573
        # make sure that when we resolve something without an extension
        # it does not throw an exception. In C++ this only issues a TF_CODING_ERROR()
        resolver = Ar.GetResolver()
        self.assertTrue(resolver.Resolve(layerIdentifier))


def default_authorize_callback(prefix):
    return (TEST_USER, TEST_PASS)
