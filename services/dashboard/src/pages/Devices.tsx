import { useState } from "react";
import { toast } from "sonner";
import { RefreshCw } from "lucide-react";

import { Button } from "@/components/ui/button";
import {
  AlertDialog,
  AlertDialogAction,
  AlertDialogCancel,
  AlertDialogContent,
  AlertDialogDescription,
  AlertDialogFooter,
  AlertDialogHeader,
  AlertDialogTitle,
} from "@/components/ui/alert-dialog";

import { useDevicesQuery } from "@/hooks/useDevicesQuery";
import { useRevokeDeviceMutation } from "@/hooks/useRevokeDeviceMutation";
import { DeviceTable } from "@/components/devices/DeviceTable";
import { DeviceDetailSheet } from "@/components/devices/DeviceDetailSheet";
import type { Device } from "@/schemas/device";

export default function Devices() {
  const { data, isLoading, isError, error, refetch, isFetching } =
    useDevicesQuery();
  const revoke = useRevokeDeviceMutation();

  const [revokeTarget, setRevokeTarget] = useState<string | null>(null);
  const [detailTarget, setDetailTarget] = useState<Device | null>(null);

  function handleConfirmRevoke() {
    if (!revokeTarget) return;
    revoke.mutate(revokeTarget, {
      onSuccess: () => toast.success(`Revoked ${revokeTarget}`),
      onError: (e) => toast.error(`Revoke failed: ${(e as Error).message}`),
      onSettled: () => setRevokeTarget(null),
    });
  }

  return (
    <div className="space-y-4">
      <header className="flex items-center justify-between">
        <h1 className="text-3xl font-bold">
          Devices{" "}
          {data && (
            <span className="ml-2 text-base text-muted-foreground">
              ({data.length})
            </span>
          )}
        </h1>
        <Button
          variant="outline"
          size="sm"
          onClick={() => refetch()}
          disabled={isFetching}
        >
          <RefreshCw
            className={`mr-2 h-4 w-4 ${isFetching ? "animate-spin" : ""}`}
          />
          Refresh
        </Button>
      </header>

      {isLoading && <p className="text-muted-foreground">Loading…</p>}
      {isError && (
        <p className="text-destructive">
          Failed to load devices: {(error as Error).message}
        </p>
      )}
      {data && (
        <DeviceTable
          devices={data}
          onRevoke={setRevokeTarget}
          onShowDetail={setDetailTarget}
        />
      )}

      <DeviceDetailSheet
        device={detailTarget}
        onClose={() => setDetailTarget(null)}
      />

      <AlertDialog
        open={revokeTarget !== null}
        onOpenChange={(o) => !o && setRevokeTarget(null)}
      >
        <AlertDialogContent>
          <AlertDialogHeader>
            <AlertDialogTitle>Revoke credential?</AlertDialogTitle>
            <AlertDialogDescription>
              Device <span className="font-mono">{revokeTarget}</span> will stop
              sending data until it is re-paired with a new code. This cannot be
              undone — the operator will need to enter a fresh 6-digit code at
              the coordinator captive portal.
            </AlertDialogDescription>
          </AlertDialogHeader>
          <AlertDialogFooter>
            <AlertDialogCancel>Cancel</AlertDialogCancel>
            <AlertDialogAction
              onClick={handleConfirmRevoke}
              className="bg-destructive text-destructive-foreground"
            >
              Revoke
            </AlertDialogAction>
          </AlertDialogFooter>
        </AlertDialogContent>
      </AlertDialog>
    </div>
  );
}
